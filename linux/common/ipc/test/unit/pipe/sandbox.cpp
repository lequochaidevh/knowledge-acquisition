#include "pipe_server.h"
#include "pipe_client.h"

using namespace HarisLinux;

int main() {
    const std::string pipe_name = "/tmp/my_test_pipe";

    // Remove all old file pipe prevent conflict
    unlink(pipe_name.c_str());
    unlink((pipe_name + "_fb").c_str());

    std::cout << "[Main] Init thread ...\n" << std::endl;

    // 1. Trigger SERVER (Mode: Feedback)
    std::thread server_thread([&pipe_name]() {
        // Init Server with mode: Feedback
        PipeServer server(pipe_name, PipeServer::ServerMode::Feedback);

        std::cout << "[Server] Waiting for Client connecting ..." << std::endl;
        server.Start();  // Block util Client connect
        std::cout << "[Server] Client connected ! Start read data ..." << std::endl;

        // Read 3 data packages when Client connect and stop
        for (int i = 0; i < 3; ++i) {
            server.ProcessIncoming();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << "[Server] Done task. Close Server thread. \n" << std::endl;
    });

    // Wait 500ms to Server have created FIFO before Client connecting.
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 2. Trigger CLIENT (Mode: Check Lose)
    std::thread client_thread([&pipe_name]() {
        // Init Client with mode: CheckLose
        PipeClient client(pipe_name, PipeClient::ClientMode::CheckLose);

        std::cout << "[Client] Connect to Server..." << std::endl;
        client.Connect();
        std::cout << "[Client] Connect successfully !" << std::endl;

        // --- TEST 1: Send data (Text) ---
        std::string          text_msg = "Hello POSIX Pipe!";
        std::vector<uint8_t> text_data(text_msg.begin(), text_msg.end());
        std::cout << "\n[Client] Send text..." << std::endl;
        client.SendData(DataType::Text, text_data);

        // Check feedback from server what if lose data appear.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        client.CheckFeedback();

        // --- TEST 2: Send data so (Number) ---
        int                  number = 2026;
        std::vector<uint8_t> num_data(sizeof(number));
        std::memcpy(num_data.data(), &number, sizeof(number));
        std::cout << "\n[Client] Send number..." << std::endl;
        client.SendData(DataType::Number, num_data);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        client.CheckFeedback();

        // --- TEST 3: Truyen Command or Media (Image/Video by byte array) ---
        std::vector<uint8_t> dummy_media = {0xFF, 0xD8, 0xFF, 0xE0, 0x01, 0x02};  // Simulate header file JPEG
        std::cout << "\n[Client] Send mock Media data..." << std::endl;
        client.SendData(DataType::Media, dummy_media);

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        client.CheckFeedback();
    });

    // Done thread
    server_thread.join();
    client_thread.join();

    // Clean up resource
    unlink(pipe_name.c_str());
    unlink((pipe_name + "_fb").c_str());

    std::cout << "[Main] Test Successfully !" << std::endl;
    return 0;
}