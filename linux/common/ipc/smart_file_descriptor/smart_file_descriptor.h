#pragma once
#include "sfd_policy.h"
#include "sfd_session_guard.h"

// --- COMPILE-TIME TAG TO FORCE LINUX SYSTEM CALL PATH ---
// Empty struct used as a compile-time signal for the constructor
struct LinuxArgs {};

template <typename Policy = FilePolicy>
class UniqueFileDescriptor {
 private:
    int                      _fd = -1;
    typename Policy::Context _ctx;  // Automatically infers the correct string/ip property pack

 public:
    UniqueFileDescriptor() = default;

    // Traditional raw fd constructor
    explicit UniqueFileDescriptor(int fd) : _fd(fd) {}

    // Variadic constructor triggered ONLY when LinuxArgs tag is passed first
    template <typename... Args>
    explicit UniqueFileDescriptor(LinuxArgs, Args&&... args) {
        // Compile-time routing based strictly on the Policy type
        if constexpr (std::is_same_v<Policy, PipePolicy> || std::is_same_v<Policy, UdpLocalhostPolicy>) {
            _fd = Policy::open_and_declare_ctx(_ctx, std::forward<Args>(args)...);
        } else {
            _fd = Policy::open(std::forward<Args>(args)...);
        }
    }

    ~UniqueFileDescriptor() { release(); }

    // Copying is prohibited to guarantee unique ownership
    UniqueFileDescriptor(const UniqueFileDescriptor&) = delete;
    UniqueFileDescriptor& operator=(const UniqueFileDescriptor&) = delete;

    // Move semantics transfer ownership safely on the stack
    UniqueFileDescriptor(UniqueFileDescriptor&& other) noexcept  //
        : _fd(other._fd),                                        //
          _ctx(std::move(other._ctx))                            //
    {
        other._fd = -1;
    }

    UniqueFileDescriptor& operator=(UniqueFileDescriptor&& other) noexcept {
        if (this != &other) {
            release();
            _fd       = other._fd;
            _ctx      = std::move(other._ctx);
            other._fd = -1;
        }
        return *this;
    }

    int      get() const { return _fd; }
    explicit operator bool() const { return _fd >= 0; }
    int      operator*() const { return _fd; }

    const typename Policy::Context& get_context() const { return _ctx; }

    void release() {
        if (_fd >= 0) {
            Policy::close(_fd, _ctx);
            _fd = -1;
        }
    }

    int release_ownership() {
        int temp_fd = _fd;
        _fd         = -1;  // Reset to prevent the destructor from calling system close()
        return temp_fd;
    }
};

// Forward declaration of the atomic registry used internally
// template <size_t MaxSlots> class StaticFileDescriptionRegistry;

template <typename Policy = FilePolicy, size_t MaxSlots = 256>
class SharedFileDescription {
 private:
    int                      _fd = -1;
    typename Policy::Context _ctx;  // Automatically infers the correct string/ip property pack

    using Registry = StaticFileDescriptionRegistry<MaxSlots>;

 public:
    SharedFileDescription() = default;

    explicit SharedFileDescription(int fd) : _fd(fd) {
        if (_fd >= 0) Registry::retain(_fd);
    }

    // Variadic constructor triggered ONLY when LinuxArgs tag is passed first
    // Completely eliminates ambiguity with copy/move/conversion constructors
    template <typename... Args>
    explicit SharedFileDescription(LinuxArgs, Args&&... args) {
        // Compile-time routing based strictly on the Policy type
        if constexpr (std::is_same_v<Policy, PipePolicy> || std::is_same_v<Policy, UdpLocalhostPolicy>) {
            _fd = Policy::open_and_declare_ctx(_ctx, std::forward<Args>(args)...);
        } else if constexpr (std::is_same_v<Policy, UnixDomainSocketPolicy>) {
            // FIXED: If the user mistakenly passed 'ctx' inside the variadic args pack,
            // we isolate and handle the raw system parameters directly to prevent forwarding objects to ::socket()
            if constexpr (sizeof...(args) == 4) {
                // Scenario: (LinuxArgs{}, ctx, domain, type, protocol) -> Extract the system flags cleanly
                _fd = UnixDomainSocketPolicy::open_receiver_with_forwarded_ctx(_ctx, std::forward<Args>(args)...);
            } else {
                // Scenario: Standard inline construction (LinuxArgs{}, path, domain, type, protocol)
                _fd = UnixDomainSocketPolicy::open_receiver(_ctx, std::forward<Args>(args)...);
            }
        } else {
            _fd = Policy::open(std::forward<Args>(args)...);
        }
        if (_fd >= 0) Registry::retain(_fd);
    }

    // Conversion constructor utilizes release_ownership() instead of pointer hacking
    explicit SharedFileDescription(UniqueFileDescriptor<Policy>&& unique_file_desc)
        : _fd(unique_file_desc.release_ownership()) {
        if (_fd >= 0) {
            Registry::retain(_fd);
        }
    }

    ~SharedFileDescription() { reset(); }

    SharedFileDescription(const SharedFileDescription& other) : _fd(other._fd) {
        if (_fd >= 0) Registry::retain(_fd);
    }

    SharedFileDescription& operator=(const SharedFileDescription& other) {
        if (this != &other) {
            reset();
            _fd = other._fd;
            if (_fd >= 0) Registry::retain(_fd);
        }
        return *this;
    }

    SharedFileDescription(SharedFileDescription&& other) noexcept : _fd(other._fd) { other._fd = -1; }

    SharedFileDescription& operator=(SharedFileDescription&& other) noexcept {
        if (this != &other) {
            reset();
            _fd       = other._fd;
            other._fd = -1;
        }
        return *this;
    }

    void reset() {
        if (_fd >= 0) {
            // If release returns true under mutex protection, safely close the descriptor
            if (Registry::release(_fd)) {
                Policy::close(_fd, _ctx);
            }
            _fd = -1;
        }
    }

    // Acquires a locked session proxy for exclusive read/write operations
    FileDescriptionSession<Policy> lock() {
        // Fetches the dedicated fine-grained mutex for this fd and returns the active session
        return FileDescriptionSession<Policy>(_fd, Registry::get_mutex(_fd));
    }

    int      get() const { return _fd; }
    size_t   use_count() const { return Registry::get_count(_fd); }
    explicit operator bool() const { return _fd >= 0; }
    int      operator*() const { return _fd; }
};
