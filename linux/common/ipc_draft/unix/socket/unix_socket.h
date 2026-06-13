#pragma once
#include "logging/logger.h"
#include "data_packet.h"
#include <cstdarg>

namespace HarisLinux {

template <typename Modes>
class UnixSocket {
 protected:
    int              _socket_fd;
    int              _address_families;  // Stores: AF_UNIX, AF_INET, AF_INET6, etc.
    int              _type;              // Stores: SOCK_DGRAM or SOCK_STREAM
    sockaddr_storage _remote_addr;       // Universal storage block large enough for any address family
    socklen_t        _remote_addr_len;   // Precise byte length of the active address structure

    uint32_t _sequence_counter;
    uint32_t _lost_packets_count;

    uint64_t get_current_timestamp_ms();

    std::mutex                   _map_mutex;
    std::map<uint32_t, uint64_t> _sent_packets;  // stored sequence_id and time to check timeout/lose

    Ipc::Generic<Modes> _modes;

 public:
    // Forces explicit validation tracking variables upon configuration instantiation
    UnixSocket(int address_families, int type, Ipc::Generic<Modes> modes)
        : _socket_fd(-1),
          _address_families(address_families),
          _type(type),
          _remote_addr_len(sizeof(sockaddr_storage)),
          _modes(modes) {
        std::memset(&_remote_addr, 0, sizeof(_remote_addr));
    }

    virtual ~UnixSocket() {
        if (_socket_fd != -1) {
            close(_socket_fd);
        }
    }

    bool initialize_socket();

    // Maps target network descriptors into the generalized storage block abstraction layer
    bool configure_address(const std::string& target, int port = 0);

    int get_socket_fd() const { return _socket_fd; }

    // =========================================================================
    // SYMMETRICAL TRANSMISSION TEMPLATE
    // Shared core function for sending raw structured buffers across networks
    // =========================================================================
    template <typename T>
    bool base_send(int target_fd, DataType type, const T& data, uint32_t seq) {
        if (target_fd == -1) return false;

        const uint8_t* ptr  = nullptr;
        uint32_t       size = 0;

        if constexpr (std::is_arithmetic_v<T>) {
            // T is Number (int, float, double, uint32_t, ...)
            ptr  = reinterpret_cast<const uint8_t*>(&data);
            size = static_cast<uint32_t>(sizeof(T));
        } else {
            // T is Container (std::vector, std::string, ...)
            ptr  = reinterpret_cast<const uint8_t*>(data.data());
            size = static_cast<uint32_t>(data.size() * sizeof(typename T::value_type));
        }

        PacketHeader header{type, size, get_current_timestamp_ms(), seq};

        // Bundle into a single payload buffer for UDP
        std::vector<uint8_t> buffer(sizeof(PacketHeader) + size);
        std::memcpy(buffer.data(), &header, sizeof(PacketHeader));
        if (size > 0 && ptr != nullptr) {
            std::memcpy(buffer.data() + sizeof(PacketHeader), ptr, size);
        }

        ssize_t sent = 0;
        if (_type == SOCK_STREAM) {
            sent = write(target_fd, buffer.data(), buffer.size());
        } else {
            sent = sendto(target_fd, buffer.data(), buffer.size(), 0, reinterpret_cast<sockaddr*>(&_remote_addr),
                          _remote_addr_len);
        }
        return sent > 0;
    }

    // =========================================================================
    // SYMMETRICAL RECEPTION LOGIC
    // Shared core function for unpacking matching payloads from network pipelines
    // =========================================================================
    bool base_receive(int source_fd, PacketHeader& out_header, std::vector<uint8_t>& out_payload);
};

}  // namespace HarisLinux
