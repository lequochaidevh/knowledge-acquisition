#include "socket_client.h"
#include <iostream>
#include <thread>
#include <chrono>

// Independent context execution worker tasked with parsing server feedback metrics
void feedback_listener_thread(SocketClient* client, bool* keep_running) {
    while (*keep_running) {
        client->check_lose_from_feedback();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

int main() {
    SocketClient client;
    std::string  server_ip = "127.0.0.1";
    int          port      = 8080;

    std::cout << "[Client] Connecting to " << server_ip << ":" << port << "...\n";
    if (!client.connect_to_server(server_ip, port)) {
        std::cerr << "[Client] Connection failed.\n";
        return 1;
    }

    bool keep_running = true;
    // Launch an async tracking listener executing concurrent feedback evaluation loops
    std::thread listener(feedback_listener_thread, &client, &keep_running);

    std::cout << "[Client] Start sending streaming data...\n";

    // Scenario 1: Transmitting Telemetry Metric Types (Number)
    double telemetry_data = 24.58;
    std::cout << "[Client] Sending Number data...\n";
    client.send_number(telemetry_data);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Scenario 2: Transmitting Structured Characters (Text)
    std::string message = "Hello Server From UDP Client";
    std::cout << "[Client] Sending Text data...\n";
    client.send_text(message);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Scenario 3: Transmitting Operational System Directives (Command Struct)
    SocketRequestPayload cmd_payload;

    std::memset(&cmd_payload, 0, sizeof(SocketRequestPayload));  // Clear buffer memory safely

    // Safely copy a text identifier into the fixed-size array
    std::strncpy(cmd_payload.client_id, "Client_A", sizeof(cmd_payload.client_id) - 1);

    cmd_payload.command = 1;  // Code value 1 implies "Register"
    std::cout << "[Client] Sending Command data...\n";
    client.send_packet(DataType::Command, reinterpret_cast<uint8_t*>(&cmd_payload), sizeof(cmd_payload));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // Scenario 4: Simulating high-frequency Media Streaming (~60 Hz throughput)
    std::cout << "[Client] Simulating high-frequency Media streaming (60 Hz)...\n";
    std::vector<uint8_t> dummy_video_frame(1024, 0xAB);  // Allocate standard 1KB dummy array buffer

    for (int i = 0; i < 30; ++i) {
        client.send_packet(DataType::Media, dummy_video_frame.data(), dummy_video_frame.size());

        // Sleep ~16.6ms to simulate real-time ~60 frames per second frequency pacing
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // Delay thread tear down to give lingering feedback cycles a window to complete
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Terminate concurrent background tracking workers clean safely
    keep_running = false;
    if (listener.joinable()) {
        listener.join();
    }

    std::cout << "[Client] Test finished safely.\n";
    return 0;
}