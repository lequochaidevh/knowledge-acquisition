#pragma once
#include "sfd_policy_command.h"

namespace HarisLinux {

// RAII Session Guard invoking compiled policy operations under mutex protection
template <typename Policy>
class FileDescriptionSession {
 private:
    int                         _fd;
    std::lock_guard<std::mutex> _lock;  // Scope-based lock tied to the slot mutex

 public:
    FileDescriptionSession(int fd, std::mutex& mutex) : _fd(fd), _lock(mutex) {}

    // Compile-time routed wrapper
    const ssize_t write(const void* buffer, size_t count) const noexcept {
        if (_fd < 0) return -1;
        // Routes directly to Policy::write at compile-time
        return Policy::write(_fd, buffer, count);
    }

    const ssize_t write_vector(const struct iovec* iov, int count) const noexcept  //
    {
        // Routes directly to Policy::write_vector at compile-time
        return Policy::write_vector(_fd, iov, count);
    }

    const ssize_t write_vector(struct msghdr& msg) const noexcept  //
    {
        // Routes directly to Policy::write_vector at compile-time
        return Policy::write_vector(_fd, msg);
    }

    // Compile-time routed read wrapper
    const ssize_t read(void* buffer, size_t count) const {
        if (_fd < 0) return -1;
        // Routes directly to Policy::read at compile-time
        return Policy::read(_fd, buffer, count);
    }

    const ssize_t read_vector(const struct iovec* iov, int count) const noexcept  //
    {
        return Policy::read_vector(_fd, iov, count);
    }
    /**
     * @brief Overloaded wrapper that cleanly accepts a reference to a msghdr.
     */
    const ssize_t read_vector(struct msghdr& msg) noexcept {
        // Extract the internal iovec array and its length directly from the msghdr
        return Policy::read_vector(_fd, msg);
    }

    const ssize_t peek_header(PacketHeader& out_header, sockaddr_storage& out_remote_addr,  //
                              socklen_t& out_addr_len) noexcept {
        return Policy::peek_header(_fd, out_header, out_remote_addr, out_addr_len);
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
