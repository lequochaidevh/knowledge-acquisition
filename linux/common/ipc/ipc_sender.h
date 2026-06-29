#pragma once
#include "ipc_metadata.h"

namespace HarisLinux {
// 1. CRTP / Static Polymorphism (Zero-Overhead)
template <typename Derived>
class IPCSenderBase {
 protected:
    UniqueFileDescriptor _unique_fd;

    IPCSenderBase() noexcept {}
    explicit IPCSenderBase(UniqueFileDescriptor fd, std::string path) noexcept : _unique_fd(std::move(fd)) {}

    template <typename T>
    bool send(DataType data_type, const T& data, const uint32_t& seq = 0) const {
        if (!_unique_fd.is_valid()) return false;

        const uint8_t* payload_ptr  = nullptr;
        uint32_t       payload_size = 0;

        using CleanedType = std::decay_t<T>;

        if constexpr (std::is_arithmetic_v<T>) {
            // TRUE Stack Allocation: Evaluated entirely at compile-time (Zero Heap Overhead)
            // We size the array exactly to sizeof(T) so it takes up only a few bytes on the stack.
            // uint8_t local_stack_buffer[sizeof(T)];
            // Well-aligned
            // std::memcpy(local_stack_buffer, &data, sizeof(T));
            // payload_ptr  = local_stack_buffer;
            payload_ptr  = reinterpret_cast<const uint8_t*>(&data);
            payload_size = static_cast<uint32_t>(sizeof(T));
        } else if constexpr (std::is_same_v<CleanedType, std::string> ||
                             std::is_same_v<CleanedType, std::vector<uint8_t>> ||
                             std::is_same_v<CleanedType, std::string_view>) {
            // Vector & String
            payload_ptr  = reinterpret_cast<const uint8_t*>(data.data());
            payload_size = static_cast<uint32_t>(data.size() * sizeof(typename T::value_type));
        } else if constexpr (std::is_same_v<CleanedType, IPCRequestPayload>) {
            payload_ptr  = reinterpret_cast<const uint8_t*>(&data);
            payload_size = static_cast<uint32_t>(sizeof(T));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported compile-time type execution for IPC Sender!");
        }

        PacketHeader header{data_type, payload_size, get_current_timestamp_ms(), seq};

        struct iovec iov[2];
        iov[0].iov_base = const_cast<PacketHeader*>(&header);
        iov[0].iov_len  = sizeof(PacketHeader);
        iov[1].iov_base = const_cast<uint8_t*>(payload_ptr);
        iov[1].iov_len  = payload_size;

        size_t  total_bytes = sizeof(PacketHeader) + payload_size;
        ssize_t sent        = static_cast<const Derived*>(this)->write_impl(iov);

        return sent == static_cast<ssize_t>(total_bytes);
    }
};

class StreamSender : public IPCSenderBase<StreamSender> {
    friend class IPCSenderBase<StreamSender>;

 public:
    explicit StreamSender(UniqueFileDescriptor target_fd) { this->_unique_fd = std::move(target_fd); }

 protected:
    ssize_t write_impl(const struct iovec* iov) const { return writev(this->_unique_fd.get(), iov, 2); }
};

class DgramSender : public IPCSenderBase<DgramSender> {
    friend class IPCSenderBase<DgramSender>;

 public:
    DgramSender(UniqueFileDescriptor target_fd, const std::string& target_path) {
        this->_unique_fd = std::move(target_fd);
        std::memset(&remote_addr, 0, sizeof(remote_addr));
        remote_addr.sun_family = AF_UNIX;
        std::strncpy(remote_addr.sun_path, target_path.c_str(), sizeof(remote_addr.sun_path) - 1);
        addr_len = sizeof(remote_addr.sun_family) + std::strlen(remote_addr.sun_path);
    }

 private:
    sockaddr_un remote_addr{};
    socklen_t   addr_len = 0;

 protected:
    ssize_t write_impl(const struct iovec* iov) const {
        struct msghdr msg {};
        msg.msg_name    = const_cast<sockaddr*>(reinterpret_cast<const sockaddr*>(&remote_addr));
        msg.msg_namelen = addr_len;
        msg.msg_iov     = const_cast<struct iovec*>(iov);
        msg.msg_iovlen  = 2;
        return sendmsg(this->_unique_fd.get(), &msg, 0);
    }
};

}  // namespace HarisLinux
