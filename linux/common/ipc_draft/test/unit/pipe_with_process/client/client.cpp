#include "pipe_server.h"
#include "pipe_client.h"

using namespace HarisLinux;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <client_id>\n", argv[0]);
        return 1;
    }
    std::string       request_pipe_path = "/tmp/main_request_pipe";
    const std::string client_path       = "client_alpha" + std::string(argv[1]);

    std::cout << "[Main] Init thread ...\n" << std::endl;

    // 2. Trigger mode CLIENT (CheckLose)
    std::thread client_thread_template([&request_pipe_path, &client_path]() {
        // Init client
        PipeClient client(request_pipe_path, client_path, PipeClient::ClientMode::CheckLose);

        // Subcribe to server
        if (client.request_and_switch_pipe()) {
            // First mess
            client.push_data(DataType::Text, std::string("Private data Client Alpha and Server!"));
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client.check_feedback();
        }
        int cnt = 2;
        while (--cnt) {
            // --- TEST 1: Send text data (Text) ---
            // Not need covert string to vector<uint8_t>, can send directly string data
            std::string text_msg = "Hello POSIX Pipe!";
            std::cout << "\n" << client_path << " Send text..." << std::endl;
            client.push_data(DataType::Text, text_msg);

            // Check lose data from Server
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client.check_feedback();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            // --- TEST 2: Send interger (Number) ---
            // Not need temporary data and memcpy,
            // Send data directly interger data structure with raw array
            // (raw array / string_view)
            int              number = 2026;
            std::string_view num_data(reinterpret_cast<const char *>(&number), sizeof(number));
            std::cout << "\n" << client_path << " Send number..." << std::endl;
            client.push_data(DataType::Number, num_data);

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client.check_feedback();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            // --- TEST 3: Send data Media/Command type (byte array) ---
            // still use std::vector<uint8_t> for large raw data, func template auto get size infomation.
            std::vector<uint8_t> dummy_media = {0xFF, 0xD8, 0xFF, 0xE0, 0x01, 0x02};
            std::cout << "\n" << client_path << " Send mock Media data..." << std::endl;
            client.push_data(DataType::Media, dummy_media);

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            client.check_feedback();
            std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        }
    });

    client_thread_template.join();

    std::cout << "[Main] Test Successfully !" << std::endl;
    return 0;
}