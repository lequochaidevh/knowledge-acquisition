#pragma once
#include "ipc_metadata.h"

namespace HarisLinux {
template <typename Derived>
class IPCReceiverBase {
 public:
    int  _fd = -1;
    bool receive(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
        if (_fd == -1) return false;
        return static_cast<Derived*>(this)->read_impl(out_header, out_payload);
    }

 protected:
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
 public:
    explicit StreamReceiver(int source_fd) { this->_fd = source_fd; }
    bool read_impl(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
        if (!this->read_all(this->_fd, &out_header, sizeof(PacketHeader))) return false;
        if (out_header.payload_size > 0) {
            out_payload.resize(out_header.payload_size);
            return this->read_all(this->_fd, out_payload.data(), out_header.payload_size);
        }
        return true;
    }
};

class DgramReceiver : public IPCReceiverBase<DgramReceiver> {
 public:
    sockaddr_storage remote_addr{};
    socklen_t        addr_len = sizeof(remote_addr);
    explicit DgramReceiver(int source_fd) { this->_fd = source_fd; }

    bool read_impl(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
        // FIXED: Unix Datagram boundaries must read the full message atomically.
        // We use a large, reusable stack buffer to fetch the packet in one single kernel hop.
        static constexpr size_t MAX_DGRAM_BUFFER = 65535;
        std::vector<uint8_t>    buffer(MAX_DGRAM_BUFFER);

        addr_len = sizeof(remote_addr);
        ssize_t received_bytes =
            recvfrom(this->_fd, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&remote_addr), &addr_len);

        if (received_bytes < static_cast<ssize_t>(sizeof(PacketHeader))) return false;

        std::memcpy(&out_header, buffer.data(), sizeof(PacketHeader));

        if (out_header.payload_size > 0) {
            out_payload.resize(out_header.payload_size);
            std::memcpy(out_payload.data(), buffer.data() + sizeof(PacketHeader), out_header.payload_size);
        }
        return true;
    }
};

}  // namespace HarisLinux
