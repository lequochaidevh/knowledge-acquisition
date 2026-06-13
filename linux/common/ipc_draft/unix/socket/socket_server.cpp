#include "socket_server.h"

namespace HarisLinux {

static auto logger = LogRegistry::getInstance().getLogger("SocketServer");

void SocketServer::add_client_if_new(const sockaddr_storage& client_addr, socklen_t addr_len) {
    for (const auto& client : _connected_clients) {
        logger->setLevel(LogLevel::Trace);

        // Compare memory blocks dynamically across family types safely
        if (std::memcmp(&client, &client_addr, addr_len) == 0) {
            return;
        }
    }
    _connected_clients.push_back(client_addr);
}

bool SocketServer::start_server(const std::string& target, int port) {
    if (!initialize_socket()) return false;
    if (!configure_address(target, port)) return false;

    if (_address_families == AF_UNIX) unlink(target.c_str());

    int opt = 1;
    setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(_socket_fd, reinterpret_cast<sockaddr*>(&_remote_addr), _remote_addr_len) < 0) return false;

    if (_type == SOCK_STREAM) {
        if (listen(_socket_fd, 10) < 0) return false;
    }
    return true;
}

bool SocketServer::accept_connection() {
    if (_type != SOCK_STREAM) return true;
    _client_fd = accept(_socket_fd, nullptr, nullptr);
    return _client_fd != -1;
}

void SocketServer::process_loss_monitoring() {
    if (!(_modes & Ipc::Server::CheckLose)) return;

    uint64_t                    now = get_current_timestamp_ms();
    std::lock_guard<std::mutex> lock(_map_mutex);

    auto it = _sent_packets.begin();
    while (it != _sent_packets.end()) {
        if (now - it->second > 200) {
            _lost_packets_count++;
            HARIS_LOG_WARN("[CheckLose] Lost Packet! Seq: {} | Total Lost: {}", it->first, _lost_packets_count);

            it = _sent_packets.erase(it);
        } else {
            ++it;
        }
    }
}

// Server Receive Wrapper
bool SocketServer::receive_packet(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
    int fd = (_type == SOCK_STREAM) ? _client_fd : _socket_fd;
    if (!base_receive(fd, out_header, out_payload)) return false;

    if (_type == SOCK_DGRAM) {
        add_client_if_new(_remote_addr, _remote_addr_len);
    }

    // DYNAMIC CHECK 1: Feedback bitmask execution
    if (_modes & Ipc::Server::Feedback) {
        std::string ack_tag = "ACK_SERVER";
        base_send(fd, DataType::Heartbeat, ack_tag, out_header.sequence_id);
    }

    // DYNAMIC CHECK 2: CheckLose bitmask execution
    if ((_modes & Ipc::Server::CheckLose) && out_header.type == DataType::Heartbeat) {
        std::lock_guard<std::mutex> lock(_map_mutex);
        _sent_packets.erase(out_header.sequence_id);
    }

    return true;
}

}  // namespace HarisLinux
