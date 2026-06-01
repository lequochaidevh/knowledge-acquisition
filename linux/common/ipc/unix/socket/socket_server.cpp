#include "socket_server.h"

bool SocketServer::start_server(int port) {
    if (!initialize_socket()) return false;

    _server_addr.sin_family      = AF_INET;
    _server_addr.sin_addr.s_addr = INADDR_ANY;
    _server_addr.sin_port        = htons(port);

    int opt = 1;
    setsockopt(_socket_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(_socket_fd, (struct sockaddr*)&_server_addr, sizeof(_server_addr)) < 0) {
        return false;
    }
    return true;
}

bool SocketServer::receive_packet(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
    uint8_t     buffer[65535];
    sockaddr_in client_addr;
    socklen_t   addr_len = sizeof(client_addr);

    ssize_t received_bytes = recvfrom(_socket_fd, buffer, sizeof(buffer), 0, (struct sockaddr*)&client_addr, &addr_len);
    if (received_bytes < static_cast<ssize_t>(sizeof(PacketHeader))) {
        return false;
    }

    std::memcpy(&out_header, buffer, sizeof(PacketHeader));
    out_payload.resize(out_header.payload_size);
    std::memcpy(out_payload.data(), buffer + sizeof(PacketHeader), out_header.payload_size);

    if (_mode == ServerMode::Broadcast || _mode == ServerMode::Feedback) {
        add_client_if_new(client_addr);
    }

    // Calc Hz base on Timestamp of all mess. Todo: calc on each message.
    uint64_t now     = get_current_timestamp_ms();
    double   latency = (now >= out_header.timestamp_ms) ? (now - out_header.timestamp_ms) : 0;
    double   hz      = latency > 0 ? (1000.0 / latency) : 0.0;

    std::cout << "[Server] Recv Type: " << static_cast<int>(out_header.type) << " | Seq: " << out_header.sequence_id
              << " | Est. Speed: " << hz << " Hz\n";

    // Feedback mode: send Heartbeat to Client check lose
    if (_mode == ServerMode::Feedback) {
        PacketHeader ack_header = {DataType::Heartbeat, 0, get_current_timestamp_ms(), out_header.sequence_id};
        sendto(_socket_fd, &ack_header, sizeof(PacketHeader), 0, (struct sockaddr*)&client_addr, addr_len);
    }

    return true;
}

void SocketServer::broadcast_packet(DataType type, const uint8_t* data, uint32_t size, uint32_t& seq_counter) {
    if (_mode != ServerMode::Broadcast || _connected_clients.empty()) return;

    PacketHeader         header = {type, size, get_current_timestamp_ms(), seq_counter++};
    std::vector<uint8_t> buffer(sizeof(PacketHeader) + size);
    std::memcpy(buffer.data(), &header, sizeof(PacketHeader));
    if (size > 0) {
        std::memcpy(buffer.data() + sizeof(PacketHeader), data, size);
    }

    for (const auto& client : _connected_clients) {
        sendto(_socket_fd, buffer.data(), buffer.size(), 0, (struct sockaddr*)&client, sizeof(client));
    }
}