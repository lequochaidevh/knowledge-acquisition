#pragma once
#include "logging/logger.h"
#include "ipc_metadata.h"
#include <chrono>

namespace HarisLinux {

class UnixSocket {
 protected:
    int         _socket_fd;
    sockaddr_in _server_addr;
    uint32_t    _sequence_counter;

    uint64_t get_current_timestamp_ms();

    std::map<uint32_t, uint64_t> _sent_packets;  // stored sequence_id and time to check timeout/lose

 public:
    UnixSocket() : _socket_fd(-1), _sequence_counter(0) { std::memset(&_server_addr, 0, sizeof(_server_addr)); }

    virtual ~UnixSocket() {
        if (_socket_fd != -1) {
            close(_socket_fd);
        }
    }

    bool initialize_socket();

    template <typename T>
    bool send_packet(DataType type, const T& data) {
        if (_socket_fd == -1) return false;

        // Fixed: Added reinterpret_cast to safely convert any container's raw data pointer to uint8_t*
        const uint8_t* ptr  = reinterpret_cast<const uint8_t*>(data.data());
        uint32_t       size = static_cast<uint32_t>(data.size() * sizeof(typename T::value_type));
        uint32_t       seq  = _sequence_counter++;

        PacketHeader header{type, size, get_current_timestamp_ms(), seq};

        // Bundle into a single payload buffer for UDP
        std::vector<uint8_t> buffer(sizeof(PacketHeader) + size);
        std::memcpy(buffer.data(), &header, sizeof(PacketHeader));
        if (size > 0 && ptr != nullptr) {
            std::memcpy(buffer.data() + sizeof(PacketHeader), ptr, size);
        }

        ssize_t sent =
            sendto(_socket_fd, buffer.data(), buffer.size(), 0, (struct sockaddr*)&_server_addr, sizeof(_server_addr));

        if (sent > 0) {
            _sent_packets[seq] = header.timestamp_ms;
            return true;
        }
        return false;
    }
};

}  // namespace HarisLinux
