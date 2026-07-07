#pragma once
#include "sfd_policy.h"

namespace HarisLinux {

// RAII Session Guard invoking compiled policy operations under mutex protection
template <typename Policy>
class FileDescriptionSession {
 private:
    int                         _fd;
    std::lock_guard<std::mutex> _lock;  // Scope-based lock tied to the slot mutex

 public:
    FileDescriptionSession(int fd, std::mutex& mutex) : _fd(fd), _lock(mutex) {}

    // Compile-time routed write wrapper
    ssize_t write(const void* buffer, size_t count) {
        if (_fd < 0) return -1;
        // Routes directly to Policy::write at compile-time
        return Policy::write(_fd, buffer, count);
    }

    // Compile-time routed read wrapper
    ssize_t read(void* buffer, size_t count) {
        if (_fd < 0) return -1;
        // Routes directly to Policy::read at compile-time
        return Policy::read(_fd, buffer, count);
    }

    // Optional: Variadic forwarding to call specialized APIs unique to certain policies
    template <typename... Args>
    ssize_t custom_io(Args&&... args) {
        if (_fd < 0) return -1;
        return Policy::send_custom(_fd, std::forward<Args>(args)...);
    }

    int get_fd() const { return _fd; }
};

}  // namespace HarisLinux
