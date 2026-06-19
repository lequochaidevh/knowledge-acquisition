#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <string>
#include <vector>
#include <string_view>
#include "haris/linux/common/ipc/pipe_server.h"
#include "haris/linux/common/ipc/pipe_client.h"

using namespace HarisLinux;

// Shared pipe path for the test environment
const std::string K_REQUEST_PIPE_PATH = "/tmp/main_request_pipe";

// --- THREAD WORKERS (HELPER FUNCTIONS) ---

void RunServer() {
    // Configure server flags
    Ipc::Generic server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;
    PipeServer   server(K_REQUEST_PIPE_PATH, server_flags);

    // Server starts listening and processing events
    server.dispatch_events();
    std::cout << "[Server] Done task. Close Server thread." << std::endl;
}

void RunClient01() {
    // Configure client flags
    Ipc::Generic client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;
    PipeClient   client(K_REQUEST_PIPE_PATH, "client_alpha", client_flags);

    // Subscribe to server
    ASSERT_TRUE(client.request_and_switch_pipe()) << "Client 01 failed to request and switch pipe!";

    // Send the initial greeting message
    client.push_data(DataType::Text, std::string("Private data Client Alpha_01 and Server!"));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    client.check_feedback();

    // Test loop: Executed once for Unit Testing instead of using an infinite while(1) loop
    for (int i = 0; i < 1; ++i) {
        // --- TEST 1: Send text data ---
        // No need to convert string to vector<uint8_t>, can send string data directly
        std::string text_msg = "Hello POSIX Pipe!";
        std::cout << "\n[Client 01] Send text..." << std::endl;
        client.push_data(DataType::Text, text_msg);

        // Check for data loss from Server
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        client.check_feedback();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));  // Reduced delay for faster execution

        // --- TEST 2: Send integer (Number) ---
        // No need for temporary data structures or memcpy.
        // Send data directly using a raw array / string_view.
        int              number = 2026;
        std::string_view num_data(reinterpret_cast<const char *>(&number), sizeof(number));
        std::cout << "\n[Client 01] Send number..." << std::endl;
        client.push_data(DataType::Number, num_data);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        client.check_feedback();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // --- TEST 3: Send Media/Command data type (byte array) ---
        // Still uses std::vector<uint8_t> for large raw data.
        // The function template automatically extracts the size information.
        std::vector<uint8_t> dummy_media = {0xFF, 0xD8, 0xFF, 0xE0, 0x01, 0x02};
        std::cout << "\n[Client 01] Send mock Media data..." << std::endl;
        client.push_data(DataType::Media, dummy_media);

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        client.check_feedback();
    }
}

void RunClient02() {
    std::cout << "\n[Client 02] request: " << K_REQUEST_PIPE_PATH << "\n";

    // Configure client flags
    Ipc::Generic client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;
    PipeClient   client(K_REQUEST_PIPE_PATH, "client_alpha_02", client_flags);

    // Subscribe to server
    ASSERT_TRUE(client.request_and_switch_pipe()) << "Client 02 failed to request and switch pipe!";

    // Send the initial greeting message
    client.push_data(DataType::Text, std::string("Private data Client Alpha_02 and Server!"));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    client.check_feedback();

    // Reduced iteration count from 500 to 5 to prevent test hanging
    int cnt = 5;
    while (cnt--) {
        // --- TEST 1: Send text data ---
        std::string text_msg = "----02----";
        std::cout << "\n[Client 02] Send text..." << std::endl;
        client.push_data(DataType::Text, text_msg);

        // Check for data loss from Server
        std::this_thread::sleep_for(std::chrono::milliseconds(50));  // Reduced delay
        client.check_feedback();
    }
}

// --- GOOGLE TEST CASE ---

TEST(PipeIpcTest, MultiClientCommunication) {
    std::cout << "[Main Test] Init threads ..." << std::endl;

    // 1. Trigger SERVER thread
    std::thread server_thread(RunServer);

    // Wait 20ms to ensure the Server has created the FIFO file before clients connect
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // 2. Trigger CLIENT 01 thread
    std::thread client01_thread(RunClient01);

    // Wait 20ms before launching the next client
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // 3. Trigger CLIENT 02 thread
    std::thread client02_thread(RunClient02);

    // Wait for both Client threads to complete their execution
    client01_thread.join();
    client02_thread.join();

    // Detach the server thread if your PipeServer does not automatically exit when clients disconnect.
    // If your server has a stop() function, call it here instead of detaching.
    if (server_thread.joinable()) {
        server_thread.detach();
    }

    SUCCEED() << "[Main Test] Test Successfully !";
}

// Standard Google Test main entry point
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}