#include "socket_client.h"

bool SocketClient::connect_to_server(const std::string& ip, int port) {
    if (!initialize_socket()) return false;

    _server_addr.sin_family = AF_INET;
    _server_addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip.c_str(), &_server_addr.sin_addr);

    // Set timeout for recvfrom function to supper-loop when check feedback
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 50000;  // 50ms timeout
    setsockopt(_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    return true;
}

void SocketClient::check_lose_from_feedback() {
    PacketHeader ack_header;
    sockaddr_in  from_addr;
    socklen_t    addr_len = sizeof(from_addr);

    // try read Feedback ACK from server
    ssize_t received =
        recvfrom(_socket_fd, &ack_header, sizeof(PacketHeader), 0, (struct sockaddr*)&from_addr, &addr_len);

    if (received >= static_cast<ssize_t>(sizeof(PacketHeader))) {
        if (ack_header.type == DataType::Heartbeat) {
            // got ACK -> Remove it out of Client's queue
            _sent_packets.erase(ack_header.sequence_id);
        }
    }

    // Check message over 200ms but can not get ACK -> Count Lose Increase
    uint64_t now = get_current_timestamp_ms();
    auto     it  = _sent_packets.begin();
    while (it != _sent_packets.end()) {
        if (now - it->second > 200) {  // over time
            _lost_packets_count++;
            std::cout << "[Client] Lost Packet Detected! Seq ID: " << it->first
                      << " | Total Lost: " << _lost_packets_count << "\n";
            it = _sent_packets.erase(it);
        } else {
            ++it;
        }
    }
}