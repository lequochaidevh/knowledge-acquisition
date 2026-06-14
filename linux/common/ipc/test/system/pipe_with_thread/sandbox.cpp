#include "haris/linux/common/ipc/pipe_server.h"
#include "haris/linux/common/ipc/pipe_client.h"

using namespace HarisLinux;

int main() {
    std::string request_pipe_path = "/tmp/main_request_pipe";

    std::cout << "[Main] Init thread ...\n" << std::endl;

    // 1. Trigger SERVER mode (Feedback)
    std::thread server_thread_template([&request_pipe_path]() {
        // Init Server
        Ipc::Generic server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;
        PipeServer   server(request_pipe_path, server_flags);
        server.dispatch_events();
        std::cout << "[Server] Done task. Close Server thread. \n" << std::endl;
    });
    // Wait 20ms to Server have created FIFO file before client connecting.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // 2. Trigger mode CLIENT (CheckLose)
    std::thread client01_thread_template([&request_pipe_path]() {
        // Init client

        Ipc::Generic client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;
        PipeClient   client(request_pipe_path, "client_alpha", client_flags);

        // Subcribe to server
        if (client.request_and_switch_pipe()) {
            // First mess
            client.push_data(DataType::Text, std::string("Private data Client Alpha_01 and Server!"));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client.check_feedback();
        }

        while (1) {
            // --- TEST 1: Send text data (Text) ---
            // Not need covert string to vector<uint8_t>, can send directly string data
            std::string text_msg = "Hello POSIX Pipe!";
            std::cout << "\n[Client 01] Send text..." << std::endl;
            client.push_data(DataType::Text, text_msg);

            // Check lose data from Server
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client.check_feedback();
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));

            // --- TEST 2: Send interger (Number) ---
            // Not need temporary data and memcpy,
            // Send data directly interger data structure with raw array
            // (raw array / string_view)
            int              number = 2026;
            std::string_view num_data(reinterpret_cast<const char*>(&number), sizeof(number));
            std::cout << "\n[Client 01] Send number..." << std::endl;
            client.push_data(DataType::Number, num_data);

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client.check_feedback();
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));

            // --- TEST 3: Send data Media/Command type (byte array) ---
            // still use std::vector<uint8_t> for large raw data, func template auto get size infomation.
            std::vector<uint8_t> dummy_media = {0xFF, 0xD8, 0xFF, 0xE0, 0x01, 0x02};
            std::cout << "\n[Client 01] Send mock Media data..." << std::endl;
            client.push_data(DataType::Media, dummy_media);

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client.check_feedback();
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
    });

    // Wait 20ms to Server have created FIFO file before client connecting.
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    std::thread client02_thread_template([&request_pipe_path]() {
        // Init client
        std::cout << "\n[Client 02] request: " << request_pipe_path << "\n";

        Ipc::Generic client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;
        PipeClient   client(request_pipe_path, "client_alpha_02", client_flags);

        // Subcribe to server
        if (client.request_and_switch_pipe()) {
            client.push_data(DataType::Text, std::string("Private data Client Alpha_02 and Server!"));

            client.check_feedback();
        }

        int cnt = 500;
        while (cnt--) {
            // --- TEST 1: Send text data (Text) ---
            // Not need covert string to vector<uint8_t>, can send directly string data
            std::string text_msg = "----02----";
            std::cout << "\n[Client 02] Send text..." << std::endl;
            client.push_data(DataType::Text, text_msg);

            // Check lose data from Server
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            client.check_feedback();
        }
    });

    // Wait 2 thread done
    server_thread_template.join();
    client01_thread_template.join();
    client02_thread_template.join();

    std::cout << "[Main] Test Successfully !" << std::endl;
    return 0;
}