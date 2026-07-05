#pragma once
#include "ipc_metadata.h"

namespace HarisLinux {
// 1. CRTP / Static Polymorphism (Zero-Overhead)
template <typename Policy>
class IPCSenderBase {
 protected:
    // Internal variable with a leading underscore tracking shared reference lifecycles
    SharedFileDescription<Policy> _shared_fd{};

    IPCSenderBase() noexcept {}
    // Explicit constructor taking the ownership model from the outside
    explicit IPCSenderBase(SharedFileDescription<Policy> fd_input) : _shared_fd(std::move(fd_input)) {}

    template <typename T>
    bool send(DataType data_type, const T& data, const uint32_t& seq = 0) const {
        if (!_shared_fd) return false;

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

        // Secure session allocation under fine-grained registry lock protection
        auto session = _shared_fd.lock();

        size_t total_bytes = sizeof(PacketHeader) + payload_size;
        // Execute the exact optimized system call at compile-time!
        ssize_t sent = Policy::write_vector(session.get_fd(), iov, _shared_fd.get_context());

        return sent == static_cast<ssize_t>(total_bytes);
    }
};

// template <typename Policy>
// class StreamSender : public IPCSenderBase<StreamSender<Policy>, Policy> {
//     friend class IPCSenderBase<StreamSender>;

//  public:
//     explicit StreamSender(SharedFileDescription<Policy> target_fd)
//         : IPCSenderBase<StreamSender<Policy>, Policy>(std::move(target_fd)) {}

//  protected:
//     // Handles continuous stream writing via writev natively across any streaming policy
//     ssize_t write_impl(int active_fd, const struct iovec* iov) const { return ::writev(active_fd, iov, 2); }
// };

// template <typename Policy>
// class DgramSender : public IPCSenderBase<DgramSender<Policy>, Policy> {
//     friend class IPCSenderBase<DgramSender<Policy>, Policy>;

//  public:
//     DgramSender(SharedFileDescription<Policy> target_fd, const std::string& target_path)
//         : IPCSenderBase<DgramSender<Policy>, Policy>(std::move(target_fd)) {
//         this->_fd_list.reset_active_fd(std::move(target_fd));
//         std::memset(&remote_addr, 0, sizeof(remote_addr));
//         remote_addr.sun_family = AF_UNIX;
//         std::strncpy(remote_addr.sun_path, target_path.c_str(), sizeof(remote_addr.sun_path) - 1);
//         addr_len = sizeof(remote_addr.sun_family) + std::strlen(remote_addr.sun_path);
//     }

//  private:
//     sockaddr_un remote_addr{};
//     socklen_t   addr_len = 0;

//  protected:
//     ssize_t write_impl(int active_fd, const struct iovec* iov) const {
//         struct msghdr msg {};
//         msg.msg_name    = const_cast<sockaddr*>(reinterpret_cast<const sockaddr*>(&remote_addr));
//         msg.msg_namelen = addr_len;
//         msg.msg_iov     = const_cast<struct iovec*>(iov);
//         msg.msg_iovlen  = 2;
//         return sendmsg(active_fd, &msg, 0);
//     }
// };

}  // namespace HarisLinux