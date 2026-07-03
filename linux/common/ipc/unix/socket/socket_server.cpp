#include "socket_server.h"

namespace HarisLinux {

// static auto logger = LogRegistry::getInstance().getLogger("SocketServer");

template <typename Transport>
void SocketServer<Transport>::add_client_if_new(const sockaddr_storage& client_addr, socklen_t addr_len) {
    for (const auto& client : _connected_clients) {
        // logger->setLevel(LogLevel::Trace);

        // Compare memory blocks dynamically across family types safely
        if (std::memcmp(&client, &client_addr, addr_len) == 0) {
            return;
        }
    }
    _connected_clients.push_back(client_addr);
}

template <typename Transport>
bool SocketServer<Transport>::start_server(const std::string& target, int port) {
    if (!this->initialize_socket()) return false;
    if (!this->configure_address(target, port)) return false;

    if (this->_address_families == AF_UNIX) ::unlink(target.c_str());

    int opt = 1;
    ::setsockopt(this->_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (::bind(this->_socket_fd,                                        //
               reinterpret_cast<const sockaddr*>(&this->_remote_addr),  //
               this->_remote_addr_len) == -1) {
        return false;
    }

    if (this->_type == SOCK_STREAM) {
        if (listen(this->_socket_fd, 10) < 0) return false;
    }
    return true;
}

template <typename Transport>
bool SocketServer<Transport>::accept_connection() {
    if (this->_type != SOCK_STREAM) return true;
    _client_fd = accept(this->_socket_fd, nullptr, nullptr);
    return _client_fd != -1;
}

template <typename Transport>
void SocketServer<Transport>::process_loss_monitoring() {
    if (!(this->_modes & Ipc::Server::CheckLose)) return;

    uint64_t                    now = get_current_timestamp_ms();
    std::lock_guard<std::mutex> lock(this->_map_mutex);

    auto it = this->_sent_packets.begin();
    while (it != this->_sent_packets.end()) {
        if (now - it->second > 200) {
            this->_lost_packets_count++;
            HARIS_LOG_WARN("[CheckLose] Lost Packet! Seq: {} | Total Lost: {}", it->first, this->_lost_packets_count);

            it = this->_sent_packets.erase(it);
        } else {
            ++it;
        }
    }
}

// Server Receive Wrapper
template <typename Transport>
bool SocketServer<Transport>::pull_data(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
    int fd = (this->_type == SOCK_STREAM) ? _client_fd : this->_socket_fd;
    if (!this->receive_packet(fd, out_header, out_payload)) return false;

    if (this->_type == SOCK_DGRAM) {
        add_client_if_new(this->_remote_addr, this->_remote_addr_len);
    }

    // DYNAMIC CHECK 1: Feedback bitmask execution
    if (this->_modes & Ipc::Server::Feedback) {
        std::string ack_tag = "ACK_SERVER";
        this->send_packet(_client_fd, DataType::Heartbeat, ack_tag, out_header.sequence_id);
    }

    // DYNAMIC CHECK 2: CheckLose bitmask execution
    if ((this->_modes & Ipc::Server::CheckLose) && out_header.type == DataType::Heartbeat) {
        std::lock_guard<std::mutex> lock(this->_map_mutex);
        this->_sent_packets.erase(out_header.sequence_id);
    }

    return true;
}

// =================================================================
template class SocketServer<SocketType::StreamTag>;
template class SocketServer<SocketType::DgramTag>;

}  // namespace HarisLinux
