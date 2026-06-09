#include "socket_server.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace HarisLinux;

int main() {
    // Instantiate Server using UDS Datagram (AF_UNIX, SOCK_DGRAM)
    // Symmetrical design allows both sending and receiving without managing streams
    // Combine flags using '|'
    Ipc::Server server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;

    SocketServer server(AF_UNIX, SOCK_DGRAM, server_flags);

    // or Mode UDP IPv4 (ReadOnly):
    // SocketServer server(AF_INET, SOCK_DGRAM, ...);

    // or Mode UDS Stream (Broadcast):
    // SocketServer server(AF_UNIX, SOCK_STREAM, ...);

    std::string socket_path = "/tmp/uds_symmetrical.sock";

    std::cout << "[Server] Launching UDS Datagram Server at: " << socket_path << "\n";
    if (!server.start_server(socket_path)) {
        std::cerr << "[Server] Fatal error: Failed to bind socket path.\n";
        return 1;
    }

    // For SOCK_DGRAM, accept_connection immediately returns true bypassively
    server.accept_connection();
    std::cout << "[Server] Active. Listening for inbound packets...\n";

    PacketHeader                        header;
    std::vector<uint8_t>                payload;
    PacketDispatcher<DataHandlerPolicy> dispatcher;

    while (true) {
        if (server.receive_packet(header, payload)) {
            std::cout << "-------------------------------------------\n";
            std::cout << "[Server RX] Recv Type: " << static_cast<int>(header.type)
                      << " | Seq ID: " << header.sequence_id << " | Payload Size: " << header.payload_size
                      << " bytes\n";

            // Inspect the inner content payload variations
            // dispatcher.dispatch(header, payload);
        }

        // Minor pacing context throttle to protect raw CPU utilization
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    return 0;
}