#pragma once
#include "ipc_metadata.h"
#include "sfd/smart_file_descriptor.h"

namespace HarisLinux {
template <typename Policy>
class IPCReceiverBase {
 protected:
    // Internal variable with a leading underscore tracking shared reference lifecycles
    SharedFileDescriptor<Policy> _shared_fd{};

    // Internal persistent storage blocks tracked safely on class layout levels for Datagrams
    mutable sockaddr_storage _remote_addr{};
    mutable socklen_t        _addr_len = sizeof(sockaddr_storage);

 public:
    IPCReceiverBase() = default;

    // Explicit constructor linking the smart descriptor instance directly
    explicit IPCReceiverBase(SharedFileDescriptor<Policy> fd_input) : _shared_fd(std::move(fd_input)) {}

    // Unified Public API to extract full network/IPC frames atomically
    bool receive(PacketHeader& out_header, std::vector<uint8_t>& out_payload) const {
        if (!_shared_fd) return false;

        // COMPILE-TIME ROUTING ENGINE: Completely replaces the old child classes!
        if constexpr (std::is_same_v<Policy, SocketStreamPathPolicy>  //
                      || std::is_same_v<Policy, PipePolicy>           //
                      || std::is_same_v<Policy, FilePolicy>) {
            // --- STREAM CORRIDOR: Correctly using Scatter-Gather vector interface for Header ---
            struct iovec header_iov[1];
            header_iov[0].iov_base = &out_header;
            header_iov[0].iov_len  = sizeof(PacketHeader);

            // Secure locking session assignment - thread-safe read pipeline isolation
            auto session = _shared_fd.lock();
            // Call the exact system interface (readv) matching your Policy signature
            ssize_t header_bytes = session.read_vector(header_iov, 1);
            if (header_bytes != static_cast<ssize_t>(sizeof(PacketHeader))) {
                return false;  // Connection closed or incomplete header
            }

            // Process payload sequentially based on the validated stream header info
            if (out_header.payload_size > 0) {
                out_payload.resize(out_header.payload_size);

                struct iovec payload_iov[1];
                payload_iov[0].iov_base = out_payload.data();
                payload_iov[0].iov_len  = out_header.payload_size;

                ssize_t payload_bytes = session.read_vector(payload_iov, 1);
                return payload_bytes == static_cast<ssize_t>(out_header.payload_size);
            }
            return false;
        } else if constexpr (std::is_same_v<Policy, SocketDgramIPv4Policy> ||  //
                             std::is_same_v<Policy, SocketDgramPathPolicy> ||
                             std::is_same_v<Policy, UdpLocalhostPolicy>) {
            // --- DATAGRAM CORRIDOR: Atomic vector processing using MSG_PEEK + recvmsg ---
            _addr_len = sizeof(_remote_addr);

            PacketHeader peek_header;
            // Secure locking session assignment - thread-safe read pipeline isolation
            auto    session    = _shared_fd.lock();
            ssize_t peek_bytes = session.peek_header(peek_header, _remote_addr, _addr_len);
            if (peek_bytes < static_cast<ssize_t>(sizeof(PacketHeader))) {
                return false;
            }

            out_payload.resize(peek_header.payload_size);

            // Zero-Copy Scatter-Gather array binding allocation on stack frame
            struct iovec iov[2];
            iov[0].iov_base = &out_header;
            iov[0].iov_len  = sizeof(PacketHeader);
            iov[1].iov_base = out_payload.data();
            iov[1].iov_len  = peek_header.payload_size;

            struct msghdr msg {};
            msg.msg_name    = &_remote_addr;
            msg.msg_namelen = _addr_len;
            msg.msg_iov     = iov;
            msg.msg_iovlen  = (peek_header.payload_size > 0) ? 2 : 1;

            size_t  total_expected = sizeof(PacketHeader) + peek_header.payload_size;
            ssize_t received_bytes = session.read_vector(msg);

            return received_bytes == static_cast<ssize_t>(total_expected);
        } else {
            static_assert(sizeof(Policy) == 0, "Unsupported compile-time execution for IPC Receiver!");
            return false;
        }
    }
};

}  // namespace HarisLinux
