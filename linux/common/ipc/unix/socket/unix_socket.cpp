#include "unix_socket.h"

namespace HarisLinux {

uint64_t UnixSocket::get_current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// Opens an un-bound OS file descriptor matching target domain topologies
bool UnixSocket::initialize_socket() {
    _socket_fd = socket(_domain, _type, 0);
    return _socket_fd != -1;
}

bool UnixSocket::configure_address(const std::string& target, int port) {
    std::memset(&_remote_addr, 0, sizeof(_remote_addr));

    if (_domain == AF_UNIX) {
        auto* addr       = reinterpret_cast<sockaddr_un*>(&_remote_addr);
        addr->sun_family = AF_UNIX;
        std::strncpy(addr->sun_path, target.c_str(), sizeof(addr->sun_path) - 1);
        _remote_addr_len = sizeof(sockaddr_un);
        return true;
    } else if (_domain == AF_INET) {
        auto* addr       = reinterpret_cast<sockaddr_in*>(&_remote_addr);
        addr->sin_family = AF_INET;
        addr->sin_port   = htons(port);
        if (inet_pton(AF_INET, target.c_str(), &addr->sin_addr) <= 0) return false;
        _remote_addr_len = sizeof(sockaddr_in);
        return true;
    } else if (_domain == AF_INET6) {
        auto* addr        = reinterpret_cast<sockaddr_in6*>(&_remote_addr);
        addr->sin6_family = AF_INET6;
        addr->sin6_port   = htons(port);
        if (inet_pton(AF_INET6, target.c_str(), &addr->sin6_addr) <= 0) return false;
        _remote_addr_len = sizeof(sockaddr_in6);
        return true;
    }
    return false;
}

bool UnixSocket::base_receive(int source_fd, PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
    if (source_fd == -1) return false;

    if (_type == SOCK_STREAM) {
        ssize_t header_bytes = read(source_fd, &out_header, sizeof(PacketHeader));
        if (header_bytes < static_cast<ssize_t>(sizeof(PacketHeader))) return false;

        out_payload.resize(out_header.payload_size);
        if (out_header.payload_size > 0) {
            read(source_fd, out_payload.data(), out_header.payload_size);
        }
    } else {
        // 1. Declare as an array, not a single byte
        uint8_t stack_buffer[1024];

        _remote_addr_len = sizeof(_remote_addr);  // Update tracking properties dynamically on incoming flows

        // 2. Pass it directly (arrays automatically decay to pointers)
        ssize_t total_bytes = recvfrom(source_fd, stack_buffer, sizeof(stack_buffer), 0,
                                       reinterpret_cast<sockaddr*>(&_remote_addr), &_remote_addr_len);

        if (total_bytes < static_cast<ssize_t>(sizeof(PacketHeader))) return false;

        std::memcpy(&out_header, stack_buffer, sizeof(PacketHeader));
        out_payload.resize(out_header.payload_size);
        if (out_header.payload_size > 0) {
            std::memcpy(out_payload.data(), stack_buffer + sizeof(PacketHeader), out_header.payload_size);
        }
    }
    return true;
}

}  // namespace HarisLinux
