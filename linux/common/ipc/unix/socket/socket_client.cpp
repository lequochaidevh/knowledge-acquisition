#include "socket_client.h"
namespace HarisLinux {

static auto logger = LogRegistry::getInstance().getLogger("SocketClient");

// bool SocketClient::connect_to_server(const std::string& target, int port) {
//     if (!initialize_socket()) return false;

//     // MANDATORY FOR UDS DATAGRAM SYMMETRY:
//     // The client must bind to its own unique path so the server can track it
//     if (_domain == AF_UNIX) {
//         sockaddr_un client_addr;
//         std::memset(&client_addr, 0, sizeof(client_addr));
//         client_addr.sun_family = AF_UNIX;

//         // Create a unique temporary file path using the client's Process ID (PID)
//         std::string client_path = "/tmp/uds_client_" + std::to_string(getpid()) + ".sock";

//         // Clean up any stale leftover sockets from previous runs
//         unlink(client_path.c_str());

//         std::strncpy(client_addr.sun_path, client_path.c_str(), sizeof(client_addr.sun_path) - 1);

//         if (bind(_socket_fd, reinterpret_cast<sockaddr*>(&client_addr), sizeof(sockaddr_un)) < 0) {
//             std::cerr << "[Client] Failed to bind internal response path.\n";
//             return false;
//         }
//     }

//     // Map the destination target address inside the structural class block
//     if (!configure_address(target, port)) return false;

//     if (_type == SOCK_STREAM) {
//         if (connect(_socket_fd, reinterpret_cast<sockaddr*>(&_remote_addr), _remote_addr_len) < 0) return false;
//     }
//     return true;
// }
bool SocketClient::connect_to_server(const std::string& target, int port) {
    if (!initialize_socket()) return false;

    // Automatically configure local bind paths if client requires feedback or checking metrics
    if (_domain == AF_UNIX && (_modes & (IPC_CLIENT_CHECK_LOSE | IPC_CLIENT_FEEDBACK))) {
        sockaddr_un client_addr;
        std::memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sun_family = AF_UNIX;

        std::string client_path = "/tmp/uds_client_" + std::to_string(getpid()) + ".sock";
        unlink(client_path.c_str());

        std::strncpy(client_addr.sun_path, client_path.c_str(), sizeof(client_addr.sun_path) - 1);
        if (bind(_socket_fd, reinterpret_cast<sockaddr*>(&client_addr), sizeof(sockaddr_un)) < 0) return false;
    }

    if (!configure_address(target, port)) return false;
    if (_type == SOCK_STREAM) {
        if (connect(_socket_fd, reinterpret_cast<sockaddr*>(&_remote_addr), _remote_addr_len) < 0) return false;
    }
    return true;
}

bool SocketClient::receive_packet(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
    if (!base_receive(_socket_fd, out_header, out_payload)) return false;

    if (_modes & IPC_CLIENT_FEEDBACK) {
        std::string ack_tag = "ACK_CLIENT";
        base_send(_socket_fd, DataType::Heartbeat, ack_tag, out_header.sequence_id);
    } else if ((_modes & IPC_CLIENT_CHECK_LOSE) && out_header.type == DataType::Heartbeat) {
        std::lock_guard<std::mutex> lock(_map_mutex);
        _sent_packets.erase(out_header.sequence_id);
    }

    return true;
}

void SocketClient::process_loss_monitoring() {
    if (!(_modes & IPC_CLIENT_CHECK_LOSE)) return;

    uint64_t                    now = get_current_timestamp_ms();
    std::lock_guard<std::mutex> lock(_map_mutex);

    auto it = _sent_packets.begin();
    while (it != _sent_packets.end()) {
        if (now - it->second > 200) {
            _lost_packets_count++;
            std::cout << "[Client CheckLose] Lost Packet! Seq: " << it->first
                      << " | Total Lost: " << _lost_packets_count << "\n";
            it = _sent_packets.erase(it);
        } else {
            ++it;
        }
    }
}

void SocketClient::check_lose_from_feedback() {
    PacketHeader ack_header;
    sockaddr_in  from_addr;
    socklen_t    addr_len = sizeof(from_addr);

    // try read Feedback ACK from server
    ssize_t received =
        recvfrom(_socket_fd, &ack_header, sizeof(PacketHeader), 0, (struct sockaddr*)&from_addr, &addr_len);

    HARIS_LOG_TRACE("Current map size tracking: {}", _sent_packets.size());

    if (received >= static_cast<ssize_t>(sizeof(PacketHeader))) {
        if (ack_header.type == DataType::Heartbeat) {
            // got ACK -> Remove it out of Client's queue
            HARIS_LOG_TRACE("ack_header.sequence_id = {}", static_cast<int>(ack_header.sequence_id));
            _sent_packets.erase(ack_header.sequence_id);
        }
    }

    // Check message over 200ms but can not get ACK -> Count Lose Increase
    uint64_t now = get_current_timestamp_ms();
    auto     it  = _sent_packets.begin();
    while (it != _sent_packets.end()) {
        if (now - it->second > 200) {  // over time
            _lost_packets_count++;

            HARIS_LOG_ERROR(
                "Lost Packet Detected! Seq ID:  {} "
                "| Total Lost: {} ",
                it->first, _lost_packets_count);

            it = _sent_packets.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace HarisLinux
