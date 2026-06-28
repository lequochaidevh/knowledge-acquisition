#pragma once
#include "ipc_metadata.h"

namespace HarisLinux {
template <typename Derived>
class IPCReceiverBase {
 protected:
    UniqueFileDescriptor _unique_fd;

    IPCReceiverBase() noexcept {}

    bool receive(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
        if (!_unique_fd.is_valid()) return false;

        return static_cast<Derived*>(this)->read_impl(out_header, out_payload);
    }

    bool read_all(int target_fd, void* buffer, size_t length) {
        size_t   total_read = 0;
        uint8_t* ptr        = reinterpret_cast<uint8_t*>(buffer);
        while (total_read < length) {
            ssize_t bytes = read(target_fd, ptr + total_read, length - total_read);
            if (bytes <= 0) return false;
            total_read += bytes;
        }
        return true;
    }
};

class StreamReceiver : public IPCReceiverBase<StreamReceiver> {
    friend class IPCReceiverBase<StreamReceiver>;

 public:
    explicit StreamReceiver(UniqueFileDescriptor source_fd) { this->_unique_fd = std::move(source_fd); }

 protected:
    bool read_impl(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
        if (!this->read_all(this->_unique_fd.get(), &out_header, sizeof(PacketHeader))) return false;
        if (out_header.payload_size > 0) {
            out_payload.resize(out_header.payload_size);
            return this->read_all(this->_unique_fd.get(), out_payload.data(), out_header.payload_size);
        }
        return true;
    }
};

class DgramReceiver : public IPCReceiverBase<DgramReceiver> {
    friend class IPCReceiverBase<DgramReceiver>;

 public:
    explicit DgramReceiver(UniqueFileDescriptor source_fd) { this->_unique_fd = std::move(source_fd); }

 protected:
    sockaddr_storage remote_addr{};
    socklen_t        addr_len = sizeof(remote_addr);

    bool read_impl(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
        addr_len = sizeof(remote_addr);

        // Step 1: PEEK at incoming packet data within Kernel space to extract payload size
        PacketHeader peek_header;
        ssize_t      peek_bytes = ::recvfrom(this->_unique_fd.get(), &peek_header, sizeof(PacketHeader), MSG_PEEK,
                                        reinterpret_cast<sockaddr*>(&remote_addr), &addr_len);

        if (peek_bytes < static_cast<ssize_t>(sizeof(PacketHeader))) return false;

        // Step 2: Resize out_payload container (0 CPU allocation cycle if old capacity fits)
        out_payload.resize(peek_header.payload_size);

        // Step 3: Zero-Copy Scatter-Gather binding
        struct iovec iov[2];
        // Stream directly into the user's out_header reference
        iov[0].iov_base = &out_header;
        iov[0].iov_len  = sizeof(PacketHeader);
        // Stream payload raw data directly into C++ vector's inner address space
        iov[1].iov_base = out_payload.data();
        iov[1].iov_len  = peek_header.payload_size;

        struct msghdr msg {};
        msg.msg_name    = &remote_addr;
        msg.msg_namelen = addr_len;
        msg.msg_iov     = iov;
        msg.msg_iovlen  = (peek_header.payload_size > 0) ? 2 : 1;

        // Step 4: Execute kernel retrieval. Data copies from NIC/Kernel direct to memory targets.
        size_t  total_expected = sizeof(PacketHeader) + peek_header.payload_size;
        ssize_t received_bytes = ::recvmsg(this->_unique_fd.get(), &msg, 0);

        return received_bytes == static_cast<ssize_t>(total_expected);
    }
};

}  // namespace HarisLinux
