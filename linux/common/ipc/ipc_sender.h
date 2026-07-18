#pragma once
#include "ipc_metadata.h"
#include "sfd/smart_file_descriptor.h"

namespace HarisLinux {
// 1. CRTP / Static Polymorphism (Zero-Overhead)
template <typename Policy>
class IPCSenderBase {
 protected:
    // Internal variable with a leading underscore tracking shared reference lifecycles
    SharedFileDescriptor<Policy> _shared_fd{};

    IPCSenderBase() noexcept {}
    // Explicit constructor taking the ownership model from the outside
    explicit IPCSenderBase(SharedFileDescriptor<Policy> fd_input) : _shared_fd(std::move(fd_input)) {}

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

        PacketHeader header{data_type, payload_size, RuntimeUtil::get_current_time_ms(), seq};

        struct iovec iov[2];
        iov[0].iov_base = const_cast<PacketHeader*>(&header);
        iov[0].iov_len  = sizeof(PacketHeader);
        iov[1].iov_base = const_cast<uint8_t*>(payload_ptr);
        iov[1].iov_len  = payload_size;

        auto   session              = _shared_fd.lock();
        int    target_fd            = session.get_fd();
        size_t total_bytes_expected = sizeof(PacketHeader) + payload_size;

        if constexpr (std::is_same_v<Policy, SocketStreamPathPolicy> || std::is_same_v<Policy, SocketDgramPathPolicy> ||
                      std::is_same_v<Policy, PipePolicy> || std::is_same_v<Policy, FilePolicy>) {
            // --- STREAM CORRIDOR: Direct vector write ---
            ssize_t sent = session.write_vector(iov, (payload_size > 0) ? 2 : 1);
            return sent == static_cast<ssize_t>(total_bytes_expected);

        } else if constexpr (std::is_same_v<Policy, SocketDgramIPv4Policy> ||
                             std::is_same_v<Policy, UdpLocalhostPolicy>) {
            // --- DATAGRAM CORRIDOR: Vector mapping via msghdr and sendmsg ---
            // Pull the pre-configured remote destination address out of your context
            auto& ctx = _shared_fd.get_context();

            struct msghdr msg {};
            // msg.msg_name    = reinterpret_cast<sockaddr*>(const_cast<sockaddr_storage*>(&ctx.remote_addr));
            // msg.msg_namelen = ctx.addr_len;
            msg.msg_name    = nullptr;
            msg.msg_namelen = 0;
            msg.msg_iov     = iov;
            msg.msg_iovlen  = (payload_size > 0) ? 2 : 1;

            ssize_t sent = session.write_vector(msg);
            return sent == static_cast<ssize_t>(total_bytes_expected);

        } else {
            static_assert(sizeof(Policy) == 0, "Unsupported compile-time execution for IPC Sender!");
            return false;
        }
    }
};

}  // namespace HarisLinux