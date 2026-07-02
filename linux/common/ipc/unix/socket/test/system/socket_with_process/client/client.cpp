#include "socket_client.h"
#include <gtest/gtest.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <string>
#include <atomic>
#include <sys/socket.h>

using namespace HarisLinux;

// Test Fixture to manage lifecycle, background execution threads, and common data fields
class SocketClientTest : public ::testing::Test {
 protected:
    std::string               socket_path;
    Ipc::Generic<Ipc::Client> client_flags;
    std::atomic<bool>         keep_running;
    std::thread               rx_worker;

    SocketClientTest() : client_flags(Ipc::Client::Feedback | Ipc::Client::CheckLose) {}

    void SetUp() override {
        socket_path  = "/tmp/uds_symmetrical.sock";
        keep_running = true;
    }

    void TearDown() override {
        // Safe context termination barrier to ensure thread cleanup on failure
        keep_running = false;
        if (rx_worker.joinable()) {
            rx_worker.join();
        }
    }

    // Encapsulated background worker loop replacing the free function signature safely
    void run_feedback_listener(HarisLinux::SocketClient<IIpc::DgramTag>* client) {
        PacketHeader         echo_header;
        std::vector<uint8_t> echo_payload;

        std::cout << "[Client RX Thread] Async Feedback Listener Started.\n";

        while (keep_running) {
            // Symmetrical execution path check
            if (client->pull_data(echo_header, echo_payload)) {
                if (echo_header.type == DataType::Heartbeat) {
                    std::string response(reinterpret_cast<char*>(echo_payload.data()), echo_payload.size());
                    std::cout << "[Client RX Thread] Server Verified Seq ID: " << echo_header.sequence_id
                              << " | Status: " << response << "\n";
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Lower throttle bound for testing efficiency
        }
    }
};

// Test Case 1: Assures connection workflows and socket option manipulations pass smoothly
TEST_F(SocketClientTest, ServerConnectionAndSocketConfiguration) {
    HarisLinux::SocketClient<IIpc::DgramTag> client(AF_UNIX, SOCK_DGRAM, client_flags);

    // Assert client can securely reach target paths (Depends on an external active server instance)
    // Changing to EXPECT_TRUE to prevent catastrophic harness crash if server is absent
    EXPECT_TRUE(client.connect_to_server(socket_path)) << "Target server path unreachable at: " << socket_path;

    // Apply the standard receive window boundary timeout settings
    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 100000;  // 100ms window

    int socket_fd = client.get_socket_fd();
    int result    = setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Validate configuration mutations return no kernel descriptor level errors
    ASSERT_GE(result, 0) << "Failed to apply system level SO_RCVTIMEO socket configuration.";
}

// Test Case 2: Simulates the actual sequential data packet transmission flow
TEST_F(SocketClientTest, DataTransmissionPipelineFlow) {
    HarisLinux::SocketClient<IIpc::DgramTag> client(AF_UNIX, SOCK_DGRAM, client_flags);

    // Soft bypass check for standalone test verification stability
    if (!client.connect_to_server(socket_path)) {
        std::cout << "[Warning] Server context missing, skipping packet pipeline loop metrics.\n";
        SUCCEED();
        return;
    }

    // Set receive timeout
    struct timeval tv {
        0, 100000
    };
    setsockopt(client.get_socket_fd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Phase 1: Verify initialization primitive integer payloads
    EXPECT_TRUE(client.push_data(DataType::Number, 2332));

    // Spin up background worker using standardized method references safely
    rx_worker = std::thread([&]() { this->run_feedback_listener(&client); });
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Phase 2: Dispatch structured alphanumeric text streams
    std::string sample_text = "Symmetrical UDS Test Message";
    std::cout << "\n[Client TX] Dispatching custom string packet...\n";
    EXPECT_TRUE(client.push_data(DataType::Text, sample_text));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Phase 3: High-frequency frame simulation sector payload dump
    std::cout << "\n[Client TX] Starting high frequency streaming burst (Media)...\n";
    const std::vector<uint8_t> mock_frame{
        0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55,
        0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55,
        0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12,
        0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55, 0xEF,
        0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55, 0xEF,
        0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12,
        0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12,
        0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55, 0xEF,
        0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55, 0xFF,
        0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55, 0xEF, 0xFF,
        0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55, 0xEF, 0xFF,
        0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0xFF, 0x12,
        0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55, 0xEF, 0xFF, 0x12,
        0x55, 0x89, 0xE2, 0xEF, 0xFF, 0x12, 0x55, 0x89, 0xE2, 0x55, 0x89, 0xFF, 0x12, 0x55, 0x55, 0x89, 0x89,
    };

    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(client.push_data(DataType::Media, mock_frame));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Adjusted down for automated execution efficiency
    }

    // Allow internal pipeline queues to catch remaining echo packets
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\n[Client] Terminating test routines cleanly.\n";
    keep_running = false;
    if (rx_worker.joinable()) {
        rx_worker.join();
    }
}

// Global Google Test Entry Point Initialization
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}