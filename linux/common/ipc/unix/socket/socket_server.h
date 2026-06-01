#pragma once
#include "unix_socket.h"

class SocketServer : public UnixSocket {
 private:
    ServerMode               _mode;
    std::vector<sockaddr_in> _connected_clients;

    void add_client_if_new(const sockaddr_in& client_addr) {
        for (const auto& client : _connected_clients) {
            if (client.sin_addr.s_addr == client_addr.sin_addr.s_addr && client.sin_port == client_addr.sin_port) {
                return;
            }
        }
        _connected_clients.push_back(client_addr);
    }

 public:
    SocketServer(ServerMode mode) : _mode(mode) {}

    bool start_server(int port);

    // API sort: get all data.
    bool receive_packet(PacketHeader& out_header, std::vector<uint8_t>& out_payload);

    // set data to all client (use for mode Broadcast)
    void broadcast_packet(DataType type, const uint8_t* data, uint32_t size, uint32_t& seq_counter);
};
