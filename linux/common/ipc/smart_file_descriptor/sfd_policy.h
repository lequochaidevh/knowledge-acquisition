#pragma once
#include "sfd_static_registry.h"

// --- INTEGRATED COMPILE-TIME POLICIES (OPEN + CLOSE) ---

// ============================================================================
// 1. FILE POLICY (GENERIC FILES, DEVICES /dev/...)
// ============================================================================
struct EmptyContext {
    // Empty context since standard files only need the integer fd
};

struct FilePolicy {
    using Context = EmptyContext;
    template <typename... Args>
    static int open(Args&&... args) {
        return ::open(std::forward<Args>(args)...);
    }

    static void close(int fd, Context& ctx) {
        if (fd >= 0) ::close(fd);
    }

    static ssize_t write(int fd, const void* buf, size_t count) { return ::write(fd, buf, count); }
    static ssize_t read(int fd, void* buf, size_t count) { return ::read(fd, buf, count); }
};

// ============================================================================
// 2. NETWORK SOCKET POLICY (TCP/UDP IP)
// ============================================================================
struct SocketPolicy {
    using Context = EmptyContext;
    template <typename... Args>
    static int open(Args&&... args) {
        return ::socket(std::forward<Args>(args)...);
    }

    static void close(int fd, Context& ctx) {
        if (fd >= 0) {
            ::shutdown(fd, SHUT_RDWR);
            ::close(fd);
        }
    }

    static ssize_t write(int fd, const void* buf, size_t count) { return ::send(fd, buf, count, 0); }
    static ssize_t read(int fd, void* buf, size_t count) { return ::recv(fd, buf, count, 0); }

    // Linux-specific: Advanced optimized socket I/O using flags (e.g., MSG_DONTWAIT, MSG_NOSIGNAL)
    static ssize_t write_flags(int fd, const void* buf, size_t count, int flags) {
        return ::send(fd, buf, count, flags);
    }
    static ssize_t read_flags(int fd, void* buf, size_t count, int flags) { return ::recv(fd, buf, count, flags); }
};

// ============================================================================
// 3. UNIX DOMAIN SOCKET POLICY (LOCAL IPC WITH PATH UNLINK)
// ============================================================================
struct UnixDomainSocketContext {
    std::string path{};  // Dynamic or SSO-driven string container for clean code
};

// Unix Domain Socket Policy with clean up separation
struct UnixDomainSocketPolicy {
    // Internal static tracking storage to handle server-side cleanup
    using Context = UnixDomainSocketContext;

    // API for Server-side instantiation (Performs critical unlink of old stale nodes)
    template <typename... Args>
    static int open_receiver(const Context& ctx, Args&&... args) {
        ::unlink(ctx.path.c_str());  // Safely evict legacy files before binding
        return ::socket(std::forward<Args>(args)...);
    }

    template <typename... Args>
    static int open_receiver_with_forwarded_ctx(Context& internal_ctx, const Context& external_ctx, int domain,
                                                int type, int protocol) {
        internal_ctx = std::move(external_ctx);  // Clone the external parameters inside the active RAII stack frame

        ::unlink(internal_ctx.path.c_str());
        return ::socket(domain, type, protocol);  // Invokes the raw system call with 100% clean integer types
    }

    // API for Client-side instantiation (Does NOT unlink the target server path)
    template <typename... Args>
    static int open_transmitter(Args&&... args) {
        return ::socket(std::forward<Args>(args)...);
    }

    static void close(int fd, Context& ctx) {
        if (fd >= 0) {
            ::shutdown(fd, SHUT_RDWR);
            ::close(fd);
            // Only clean up the file node if this instance was registered as the server owner
            if (!ctx.path.empty()) {
                ::unlink(ctx.path.c_str());
                ctx.path.clear();
            }
        }
    }

    static ssize_t write(int fd, const void* buf, size_t count) { return ::send(fd, buf, count, 0); }
    static ssize_t read(int fd, void* buf, size_t count) { return ::recv(fd, buf, count, 0); }
};

// ============================================================================
// 4. UDP LOCALHOST POLICY (HIGH-PERFORMANCE IP IPC WITHOUT HEAP)
// ============================================================================
struct UdpContext {
    std::string ip{};
    uint16_t    port = 0;
};

struct UdpLocalhostPolicy {
    using Context = UdpContext;

    static int open_and_declare_ctx(Context& ctx, std::string& ip_address, uint16_t port) {
        ctx.ip   = std::move(ip_address);
        ctx.port = port;
        return ::socket(AF_INET, SOCK_DGRAM, 0);
    }

    static void close(int fd, Context& ctx) {
        if (fd >= 0) {
            // UDP is connectionless but we close the descriptor to free kernel resources
            ::close(fd);
        }
    }

    // Standard POSIX sendto wrapper mapped to write
    static ssize_t write(int fd, const void* buf, size_t count) {
        struct sockaddr_in dest_addr {};
        dest_addr.sin_family      = AF_INET;
        dest_addr.sin_port        = htons(9999);
        dest_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        return ::sendto(fd, buf, count, 0, reinterpret_cast<struct sockaddr*>(&dest_addr), sizeof(dest_addr));
    }

    // Standard POSIX recvfrom wrapper mapped to read
    static ssize_t read(int fd, void* buf, size_t count) { return ::recvfrom(fd, buf, count, 0, nullptr, nullptr); }
};

// ============================================================================
// 5. PIPE & FIFO POLICY (NAMED / UNNAMED PIPES WITH PATH UNLINK)
// ============================================================================
struct PipeContext {
    std::string path{};  // Dynamic or SSO-driven string container for clean code
    int         flag;    // Dynamic or SSO-driven string container for clean code
};

struct PipePolicy {
    using Context = PipeContext;

    // Creates the underlying FIFO special node on the filesystem layout
    static bool open_and_declare_ctx(Context& ctx, std::string& path, uint16_t flag) {
        ctx.path = std::move(path);
        ctx.flag = static_cast<int>(flag);
        ::unlink(ctx.path.c_str());  // Evict preexisting stale files
        return ::mkfifo(ctx.path.c_str(), 0666) == 0;
    }

    // Opens a clean streaming descriptor with explicit Linux flags
    static int open_channel(const Context& ctx, int flags) { return ::open(ctx.path.c_str(), flags); }

    static void close(int fd, Context& ctx) {
        if (fd >= 0) {
            ::close(fd);
            // Only trigger filesystem unlink if static registry reference tracking hits zero
            if (!ctx.path.empty()) {
                ::unlink(ctx.path.c_str());
                ctx.path.clear();
            }
        }
    }

    static ssize_t write(int fd, const void* buf, size_t count) { return ::write(fd, buf, count); }
    static ssize_t read(int fd, void* buf, size_t count) { return ::read(fd, buf, count); }
};

// ============================================================================
// 6. EPOLL POLICY (HIGH-PERFORMANCE LINUX EVENT MULTIPLEXING)
// ============================================================================
struct EpollPolicy {
    using Context = EmptyContext;
    static int open(int flags = 0) {
        // Modern epoll_create1 supports close-on-exec (EPOLL_CLOEXEC) optimization flag
        return ::epoll_create1(flags);
    }

    static void close(int fd, Context& ctx) {
        if (fd >= 0) ::close(fd);
    }

    // Modern Linux optimization API wrapper to register, modify, or remove monitoring nodes
    static int control(int epoll_fd, int operation, int target_fd, struct epoll_event* event) {
        return ::epoll_ctl(epoll_fd, operation, target_fd, event);
    }

    // High-performance asynchronous interface waiting for specific system triggers
    static int wait(int epoll_fd, struct epoll_event* events, int maxevents, int timeout) {
        return ::epoll_wait(epoll_fd, events, maxevents, timeout);
    }
};

// ============================================================================
// 7. EVENTFD POLICY (LIGHTWEIGHT INTER-THREAD / KERNEL SIGNALING)
// ============================================================================
struct EventFdPolicy {
    using Context = EmptyContext;
    static int open(unsigned int initval = 0, int flags = EFD_CLOEXEC | EFD_NONBLOCK) {
        return ::eventfd(initval, flags);
    }

    static void close(int fd, Context& ctx) {
        if (fd >= 0) ::close(fd);
    }

    // Writes an 8-byte integer signal to wake up listening threads instantly
    static ssize_t notify(int fd, uint64_t counter) { return ::write(fd, &counter, sizeof(counter)); }

    // Reads the 8-byte signaling information counter block
    static ssize_t fetch_signal(int fd, uint64_t& counter) { return ::read(fd, &counter, sizeof(counter)); }
};
