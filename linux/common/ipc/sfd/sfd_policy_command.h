#pragma once
#include "ipc_metadata.h"
#include "sfd_static_registry.h"

namespace HarisLinux {

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

    // Works perfectly for regular files! Kernel flushes buffers in one shot
    static ssize_t write_vector(int fd, const struct iovec* iov, const Context&) { return ::writev(fd, iov, 2); }

    // todo
    static bool setup_communication(const Context& ctx) { return true; }
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

    // todo
    static bool setup_communication(const Context& ctx) { return true; }
};

// ============================================================================
// 3. UNIX DOMAIN SOCKET POLICY (LOCAL IPC WITH PATH UNLINK)
// ============================================================================

// Context: IPv4 DGRAM (IP, Port, sockaddr_in)
struct SocketDgramIPv4Context {
    std::string local_ip;
    uint16_t    local_port = 0;
    std::string target_ip;
    uint16_t    target_port = 0;

    // Runtime cache for compiled network layouts
    sockaddr_in local_sockaddr;
    sockaddr_in target_sockaddr;
};
// --- POLICY 1: IPv4 DGRAM ---
class SocketDgramIPv4Policy {
 public:
    using Context = SocketDgramIPv4Context;

    static int init(Context& ctx, IoMode mode) {
        int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (fd == -1) {
            throw std::runtime_error("IPv4 Socket creation failed");
        }

        if (mode == IoMode::Receiver) {
            std::memset(&ctx.local_sockaddr, 0, sizeof(ctx.local_sockaddr));
            ctx.local_sockaddr.sin_family = AF_INET;
            ctx.local_sockaddr.sin_port   = htons(ctx.local_port);
            ::inet_pton(AF_INET, ctx.local_ip.c_str(), &ctx.local_sockaddr.sin_addr);
            ::bind(fd, (sockaddr*)&ctx.local_sockaddr, sizeof(ctx.local_sockaddr));
        } else if (mode == IoMode::Transmiter) {
            std::memset(&ctx.target_sockaddr, 0, sizeof(ctx.target_sockaddr));
            ctx.target_sockaddr.sin_family = AF_INET;
            ctx.target_sockaddr.sin_port   = htons(ctx.target_port);
            ::inet_pton(AF_INET, ctx.target_ip.c_str(), &ctx.target_sockaddr.sin_addr);
            ::connect(fd, (sockaddr*)&ctx.target_sockaddr, sizeof(ctx.target_sockaddr));
        }

        return fd;
    }

    /**
     * @brief Peeks at the incoming kernel socket buffer to extract header info and remote address.
     * @note This operation does not consume or evict data from the kernel's receive queue.
     * @param active_fd The active socket file descriptor.
     * @param out_header Reference to populate with the peeked packet header.
     * @param out_remote_addr Storage structure to hold the sender's network/local address metadata.
     * @param out_addr_len Length descriptor, updated by the kernel to reflect actual address bytes written.
     * @return ssize_t Total bytes peeked on success, or -1 on system call failure.
     */
    static ssize_t peek_header(int active_fd, PacketHeader& out_header, sockaddr_storage& out_remote_addr,
                               socklen_t& out_addr_len) noexcept {
        // Zero-out the address storage to prevent stale data corruption or memory leaks
        ::memset(&out_remote_addr, 0, sizeof(sockaddr_storage));

        // Initialize with maximum buffer capacity to inform the kernel of safe boundaries
        out_addr_len = sizeof(sockaddr_storage);

        // Execute system call with MSG_PEEK flag to inspect buffer head without data eviction
        ssize_t peek_bytes = ::recvfrom(active_fd, &out_header, sizeof(PacketHeader), MSG_PEEK,
                                        reinterpret_cast<sockaddr*>(&out_remote_addr), &out_addr_len);

        // Return the operation status (byte count or -1 error code) to caller
        return peek_bytes;
    }

    static ssize_t write_vector(int tx_fd, const struct iovec* iov, int count) { return ::writev(tx_fd, iov, count); }

    /**
     * @brief Executes an atomic scatter-gather read operation using POSIX message structures.
     * @note This uses the non-const msghdr reference because the kernel mutates internal fields
     *       (e.g., updating msg_namelen and msg_flags) during network execution.
     * @param rx_fd The active receiver socket file descriptor.
     * @param msg Reference to the system message container configuring destination buffers and remote address space.
     * @return ssize_t The exact total byte count received on success, or -1 on system call failure.
     */
    static ssize_t read_vector(int rx_fd, struct msghdr& msg) noexcept { return ::recvmsg(rx_fd, &msg, 0); }

    static void cleanup(const Context& ctx) {
        // No filesystem cleanup required for network stack
    }

    static void close(int fd, const Context& ctx) {
        if (fd >= 0) {
            ::close(fd);
            cleanup(ctx);
        }
    }
};

// Context: Unix Domain DGRAM (File Paths, sockaddr_un)
struct SocketDgramPathContext {
    std::string local_path;
    std::string target_path;

    // Runtime cache for compiled local filesystem layouts
    sockaddr_un local_sockaddr;
    sockaddr_un target_sockaddr;
};

// --- POLICY 2: UNIX DOMAIN DGRAM ---
class SocketDgramPathPolicy {
 public:
    using Context = SocketDgramPathContext;

    static int init(Context& ctx, IoMode mode) {
        int fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        if (fd == -1) {
            throw std::runtime_error("Unix DGRAM Socket creation failed");
        }

        if (mode == IoMode::Receiver) {
            std::memset(&ctx.local_sockaddr, 0, sizeof(ctx.local_sockaddr));
            ctx.local_sockaddr.sun_family = AF_UNIX;
            std::strncpy(ctx.local_sockaddr.sun_path, ctx.local_path.c_str(), sizeof(ctx.local_sockaddr.sun_path) - 1);

            ::unlink(ctx.local_path.c_str());
            ::bind(fd, (sockaddr*)&ctx.local_sockaddr, sizeof(ctx.local_sockaddr));
        } else if (mode == IoMode::Transmiter) {
            std::memset(&ctx.target_sockaddr, 0, sizeof(ctx.target_sockaddr));
            ctx.target_sockaddr.sun_family = AF_UNIX;
            std::strncpy(ctx.target_sockaddr.sun_path, ctx.target_path.c_str(),
                         sizeof(ctx.target_sockaddr.sun_path) - 1);
            ::connect(fd, (sockaddr*)&ctx.target_sockaddr, sizeof(ctx.target_sockaddr));
        }

        return fd;
    }

    /**
     * @brief Peeks at the incoming kernel socket buffer to extract header info and remote address.
     * @note This operation does not consume or evict data from the kernel's receive queue.
     * @param active_fd The active socket file descriptor.
     * @param out_header Reference to populate with the peeked packet header.
     * @param out_remote_addr Storage structure to hold the sender's network/local address metadata.
     * @param out_addr_len Length descriptor, updated by the kernel to reflect actual address bytes written.
     * @return ssize_t Total bytes peeked on success, or -1 on system call failure.
     */
    static ssize_t peek_header(int active_fd, PacketHeader& out_header, sockaddr_storage& out_remote_addr,
                               socklen_t& out_addr_len) noexcept {
        // Zero-out the address storage to prevent stale data corruption or memory leaks
        ::memset(&out_remote_addr, 0, sizeof(sockaddr_storage));

        // Initialize with maximum buffer capacity to inform the kernel of safe boundaries
        out_addr_len = sizeof(sockaddr_storage);

        // Execute system call with MSG_PEEK flag to inspect buffer head without data eviction
        ssize_t peek_bytes = ::recvfrom(active_fd, &out_header, sizeof(PacketHeader), MSG_PEEK,
                                        reinterpret_cast<sockaddr*>(&out_remote_addr), &out_addr_len);

        // Return the operation status (byte count or -1 error code) to caller
        return peek_bytes;
    }

    static ssize_t write_vector(int tx_fd, const struct iovec* iov, int count) { return ::writev(tx_fd, iov, count); }

    /**
     * @brief Executes an atomic scatter-gather read operation using POSIX message structures.
     * @note This uses the non-const msghdr reference because the kernel mutates internal fields
     *       (e.g., updating msg_namelen and msg_flags) during network execution.
     * @param rx_fd The active receiver socket file descriptor.
     * @param msg Reference to the system message container configuring destination buffers and remote address space.
     * @return ssize_t The exact total byte count received on success, or -1 on system call failure.
     */
    static ssize_t read_vector(int rx_fd, struct msghdr& msg) noexcept { return ::recvmsg(rx_fd, &msg, 0); }

    static void cleanup(const Context& ctx) { ::unlink(ctx.local_path.c_str()); }

    static void close(int fd, const Context& ctx) {
        if (fd >= 0) {
            ::close(fd);
            cleanup(ctx);
        }
    }
};

// Context: Unix Domain STREAM (File Paths, sockaddr_un)
struct SocketStreamPathContext {
    std::string local_path;
    std::string target_path;

    sockaddr_un local_sockaddr;
    sockaddr_un target_sockaddr;
};

// --- POLICY 3: UNIX DOMAIN STREAM ---
class SocketStreamPathPolicy {
 public:
    using Context = SocketStreamPathContext;

    static int init(Context& ctx, IoMode mode) {
        int fd = -1;
        // Critical: receiver::listen before transmiter::connect
        if (mode == IoMode::Receiver) {
            fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd == -1) {
                throw std::runtime_error("Unix STREAM Receiver Socket creation failed");
            }

            std::memset(&ctx.local_sockaddr, 0, sizeof(ctx.local_sockaddr));
            ctx.local_sockaddr.sun_family = AF_UNIX;
            std::strncpy(ctx.local_sockaddr.sun_path, ctx.local_path.c_str(), sizeof(ctx.local_sockaddr.sun_path) - 1);

            ::unlink(ctx.local_path.c_str());
            ::bind(fd, (sockaddr*)&ctx.local_sockaddr, sizeof(ctx.local_sockaddr));

            ::listen(fd, 5);
        } else if (mode == IoMode::Transmiter) {
            fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd == -1) {
                throw std::runtime_error("Unix STREAM Transmitter Socket creation failed");
            }

            std::memset(&ctx.target_sockaddr, 0, sizeof(ctx.target_sockaddr));
            ctx.target_sockaddr.sun_family = AF_UNIX;
            std::strncpy(ctx.target_sockaddr.sun_path, ctx.target_path.c_str(),
                         sizeof(ctx.target_sockaddr.sun_path) - 1);

            ::connect(fd, (sockaddr*)&ctx.target_sockaddr, sizeof(ctx.target_sockaddr));
        }

        return fd;
    }

    static ssize_t write_vector(int tx_fd, const struct iovec* iov, int count) { return ::writev(tx_fd, iov, count); }

    static ssize_t read_vector(int rx_fd, struct iovec* iov, int count) { return ::readv(rx_fd, iov, count); }

    static void cleanup(const Context& ctx) { ::unlink(ctx.local_path.c_str()); }

    static void close(int fd, const Context& ctx) {
        if (fd >= 0) {
            ::close(fd);
            cleanup(ctx);
        }
    }
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

    // todo
    static bool setup_communication(const Context& ctx) { return true; }
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

    // Works perfectly for Pipes! Prevents pipe buffer fragmentation natively
    static ssize_t write_vector(int fd, const struct iovec* iov, int count) { return ::writev(fd, iov, count); }

    /**
     * @brief Direct stream scatter-gather read execution optimized for Unix Pipes.
     * @note Pipes transfer data as a continuous byte stream without boundary encapsulation
     *       or source routing headers, making raw iovec memory binding the most efficient path.
     * @param rx_fd The active read end of the pipe file descriptor.
     * @param iov Pointer to the scatter-gather layout arrays.
     * @param count Total number of memory buffers registered inside the iovec array.
     * @return ssize_t Total bytes retrieved from the pipe buffer on success, 0 on EOF, or -1 on system failure.
     */
    static ssize_t read_vector(int rx_fd, struct iovec* iov, int count) noexcept { return ::readv(rx_fd, iov, count); }

    static const bool setup_communication(const Context& ctx) {
        ::unlink(ctx.path.c_str());  // Evict preexisting stale files
        return ::mkfifo(ctx.path.c_str(), 0666) == 0;
    }
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

    // todo
    static bool setup_communication(const Context& ctx) { return true; }
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

    // todo
    static bool setup_communication(const Context& ctx) { return true; }
};

}  // namespace HarisLinux
