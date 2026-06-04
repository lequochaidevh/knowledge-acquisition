#include "socket_server.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace HarisLinux;

int main() {
    // Instantiate Server using UDS Datagram (AF_UNIX, SOCK_DGRAM)
    // Symmetrical design allows both sending and receiving without managing streams
    // Combine flags using '|'
    uint8_t server_flags = IPC_SERVER_FEEDBACK | IPC_SERVER_CHECK_LOSE;

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

    PacketHeader         header;
    std::vector<uint8_t> payload;

    while (true) {
        if (server.receive_packet(header, payload)) {
            std::cout << "-------------------------------------------\n";
            std::cout << "[Server RX] Recv Type: " << static_cast<int>(header.type)
                      << " | Seq ID: " << header.sequence_id << " | Payload Size: " << header.payload_size
                      << " bytes\n";

            // Inspect the inner content payload variations
            if (header.type == DataType::Text) {
                std::string text_msg(reinterpret_cast<char*>(payload.data()), payload.size());
                std::cout << ">> Content Data: \"" << text_msg << "\"\n";
            } else if (header.type == DataType::Media) {
                std::cout << ">> Content Data: Raw Image/Video Stream Frame Segment\n";
            }

            // Symmetrical Check Mismatch: Echo feedback ACK back to client immediately.
            // Because base_receive captured the source properties, server knows where to fire back.
            // std::string ack_payload  = "ACK_OK";
            // bool        echo_success = server.send_packet(DataType::Heartbeat, ack_payload, header.sequence_id);

            // if (echo_success) {
            //     std::cout << "[Server TX] Successfully echoed matching feedback for Seq: " << header.sequence_id
            //               << "\n";
            // } else {
            //     std::cerr << "[Server TX] Warning: Failed to echo packet feedback back to client.\n";
            // }
        }

        // Minor pacing context throttle to protect raw CPU utilization
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }

    return 0;
}