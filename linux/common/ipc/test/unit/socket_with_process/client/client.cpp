#include "socket_client.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace HarisLinux;

// Independent concurrent background context to listen for server mismatch/heartbeat echoes
void async_feedback_listener(SocketClient* client, bool* keep_running) {
    PacketHeader         echo_header;
    std::vector<uint8_t> echo_payload;

    std::cout << "[Client RX Thread] Async Feedback Listener Started.\n";

    while (*keep_running) {
        // Symmetrical call: Client captures packets arriving back on its own file descriptor
        if (client->receive_packet(echo_header, echo_payload)) {
            if (echo_header.type == DataType::Heartbeat) {
                std::string response(reinterpret_cast<char*>(echo_payload.data()), echo_payload.size());
                std::cout << "[Client RX Thread] Server Verified Seq ID: " << echo_header.sequence_id
                          << " | Status: " << response << "\n";
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

int main() {
    // Instantiate matching UDS Datagram schema parameters
    uint8_t client_flags = IPC_CLIENT_FEEDBACK | IPC_CLIENT_CHECK_LOSE;

    SocketClient client(AF_UNIX, SOCK_DGRAM, client_flags);
    std::string  socket_path = "/tmp/uds_symmetrical.sock";

    std::cout << "[Client] Connecting to target server at: " << socket_path << "\n";
    if (!client.connect_to_server(socket_path)) {
        std::cerr << "[Client] Fatal error: Target server path unreachable.\n";
        return 1;
    }

    // Clean Timeout Configuration Block
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 100000;  // 100ms receive timeout window

    // Extract the valid underlying file descriptor using the getter
    int socket_fd = client.get_socket_fd();

    // Apply the receive timeout option directly to the socket descriptor
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        std::cerr << "[Client] Warning: Failed to set socket receive timeout.\n";
    }

    // Send initialization message normally on its own line
    client.send_packet(DataType::Text, std::string("init"));

    bool keep_running = true;
    // Launch the listener thread passing the direct object reference pointer safely
    std::thread rx_worker(async_feedback_listener, &client, &keep_running);

    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Phase 1: Sending generic structured text sequences
    std::string sample_text = "Symmetrical UDS Test Message";
    std::cout << "\n[Client TX] Dispatching custom string packet...\n";
    client.send_packet(DataType::Text, sample_text);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Phase 2: Simulating high frequency transmission flows
    std::cout << "\n[Client TX] Starting high frequency streaming burst (Media)...\n";
    std::vector<uint8_t> mock_frame(512, 0xEF);  // Allocate a clean 512-byte payload mock sector

    for (int i = 0; i < 5; ++i) {
        client.send_packet(DataType::Media, mock_frame);
        std::this_thread::sleep_for(std::chrono::milliseconds(30));  // Frame interval delay pacing
    }

    // Allow lingering server echoes time to clear the network buffer pipelines
    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "\n[Client] Terminating test routines cleanly.\n";
    keep_running = false;
    if (rx_worker.joinable()) {
        rx_worker.join();
    }

    return 0;
}