#pragma once
#include "sfd_policy_command.h"
#include "sfd_session_guard.h"

namespace HarisLinux {
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
class SharedFileDescriptor {
 private:
    int                      _fd = -1;
    typename Policy::Context _ctx;  // Automatically infers the correct string/ip property pack

    using Registry = StaticFileDescriptionRegistry<MaxSlots>;

 public:
    SharedFileDescriptor() = default;

    explicit SharedFileDescriptor(int fd) : _fd(fd) {
        if (_fd >= 0) Registry::retain(_fd);
    }

    // Variadic constructor triggered ONLY when LinuxArgs tag is passed first
    // Completely eliminates ambiguity with copy/move/conversion constructors
    template <typename... Args>
    explicit SharedFileDescriptor(LinuxArgs, Args&&... args) {
        if constexpr (std::is_same_v<Policy, PipePolicy> || std::is_same_v<Policy, UdpLocalhostPolicy>) {
            _fd = Policy::open_and_declare_ctx(_ctx, std::forward<Args>(args)...);
        } else {
            _fd = Policy::open(std::forward<Args>(args)...);
        }
        if (_fd >= 0) Registry::retain(_fd);
    }

    // Conversion constructor utilizes release_ownership() instead of pointer hacking
    explicit SharedFileDescriptor(UniqueFileDescriptor<Policy>&& unique_file_desc)
        : _fd(unique_file_desc.release_ownership()) {
        if (_fd >= 0) {
            Registry::retain(_fd);
        }
    }

    ~SharedFileDescriptor() { reset(); }

    SharedFileDescriptor(const SharedFileDescriptor& other) : _fd(other._fd) {
        if (_fd >= 0) Registry::retain(_fd);
    }

    SharedFileDescriptor& operator=(const SharedFileDescriptor& other) {
        if (this != &other) {
            reset();
            _fd = other._fd;
            if (_fd >= 0) Registry::retain(_fd);
        }
        return *this;
    }

    SharedFileDescriptor(SharedFileDescriptor&& other) noexcept : _fd(other._fd) { other._fd = -1; }

    SharedFileDescriptor& operator=(SharedFileDescriptor&& other) noexcept {
        if (this != &other) {
            reset();
            _fd       = other._fd;
            other._fd = -1;
        }
        return *this;
    }

    constexpr int release() noexcept {
        int tmp = _fd;
        if (_fd >= 0) {
            // Decrement reference count; registry filters whether to execute system close()
            (void)Registry::release(_fd);
            _fd = -1;  // Reset to stop destructor side-effects
        }
        return tmp;
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
    FileDescriptionSession<Policy> lock() const {
        // Fetches the dedicated fine-grained mutex for this fd and returns the active session
        return FileDescriptionSession<Policy>(_fd, Registry::get_mutex(_fd));
    }

    // This allows seamless zero-overhead inspection of network IPs, ports, or FIFO paths.
    const typename Policy::Context& get_context() const noexcept { return _ctx; }

    bool set_context(typename Policy::Context& ctx) noexcept {
        _ctx = std::move(ctx);
        return true;
    }

    static const bool setup_communication(const typename Policy::Context& ctx) noexcept {
        return Policy::setup_communication(ctx);
    }

    int      get() const { return _fd; }
    size_t   use_count() const { return Registry::get_count(_fd); }
    explicit operator bool() const { return _fd >= 0; }
    int      operator*() const { return _fd; }
};

/**
 * @brief Thread-safe context switch guard that generates and manages its own locks internally.
 * @details Eliminates the need to declare std::mutex instances inside base or derived classes.
 * @tparam Policy The structural policy dictating the pipe behaviors.
 */
template <typename Policy>
class SharedFdSwitchGuard {
 private:
    // Reference to the active member variable of the target instance
    SharedFileDescriptor<Policy>& _current_fd;
    SharedFileDescriptor<Policy>& _new_fd;

    // This must be a clean value object to hold the server's main fd safely on stack
    SharedFileDescriptor<Policy> _temporary_fd;

    // Explicit manual lock management instead of std::lock_guard for precise structural binding
    std::mutex* _allocated_mutex_ptr;

 public:
    /**
     * @brief Fetches or constructs a dedicated mutex associated with the target reference address.
     * @param target_address The memory address of the tracking member variable.
     * @return std::mutex& A reference to a thread-isolated synchronization primitive.
     */
    static std::mutex& get_mutex_pool(void* target_address) {
        // Global registry protecting the instantiation of dynamic locks
        static std::mutex                            pool_manager_mtx;
        static std::unordered_map<void*, std::mutex> lock_registry;

        std::lock_guard<std::mutex> pool_lock(pool_manager_mtx);
        return lock_registry[target_address];  // Automatically creates a new mutex if it doesn't exist
    }

    /**
     * @brief Construct a new Fd Switch Guard object, dynamically lock via address context, and swap.
     * @param active The reference to the core variable to be modified.
     * @param target The target descriptor to be injected into execution.
     */
    SharedFdSwitchGuard(SharedFileDescriptor<Policy>& active, SharedFileDescriptor<Policy>& target)  //
        : _current_fd(active), _new_fd(target) {
        // 1. Resolve and extract the specific lock based on the instance variable address
        _allocated_mutex_ptr = &get_mutex_pool(static_cast<void*>(&_current_fd));

        // 2. Enforce atomic isolation before executing the reference rewrite
        _allocated_mutex_ptr->lock();

        // 3. Preserve the structural backup token and apply move semantics
        _temporary_fd = std::move(_current_fd);
        _current_fd   = std::move(_new_fd);
    }

    /**
     * @brief Destroy the Fd Switch Guard object, restore context, and release the internal lock.
     */
    ~SharedFdSwitchGuard() {
        // 1. Unconditionally restore original system state
        _new_fd     = std::move(_current_fd);
        _current_fd = std::move(_temporary_fd);

        // 2. Release the isolated instance mutex safely on scope departure
        if (_allocated_mutex_ptr != nullptr) {
            _allocated_mutex_ptr->unlock();
        }
    }
};

}  // namespace HarisLinux
