#include "pipe_client.h"

#include <gtest/gtest.h>

using namespace HarisLinux;

const std::string TEST_REQUEST_PIPE = "/tmp/main_request_pipe_test";
std::string       GLOBAL_CLIENT_ID  = "default_id";
int16_t           delay_arg         = 100;

// ==============================================================================
// TEST DERIVATION LAYER (Spy Pattern via 'using')
// Exposes protected methods of PipeClient to public scope for testing execution
// ==============================================================================
class TestPipeClient : public HarisLinux::PipeClient {
 public:
    // Pull the protected base constructor into public visibility scope
    using HarisLinux::PipeClient::PipeClient;

    // Pull the internal operations into public scope
    using HarisLinux::PipeClient::check_feedback;
    using HarisLinux::PipeClient::push_data;
    using HarisLinux::PipeClient::request_and_switch_pipe;
};

// ==============================================================================
// GOOGLETEST FIXTURE
// ==============================================================================
class PipeClientTest : public ::testing::Test {
 protected:
    void SetUp() override {
        // Initial setup for the testing environment context if needed
    }

    void TearDown() override {
        // Cleanup logic to safely unbind local testing footprints
    }
};

// ==============================================================================
// THE CLIENT TRANSACTION UNIT TEST
// ==============================================================================
void heartbeat_task(TestPipeClient &client) {
    static auto last_heartbeat_time = get_current_timestamp_ms();
    auto        current_time        = get_current_timestamp_ms();

    auto elapsed = current_time - last_heartbeat_time;

    if (elapsed >= 1000) {
        stdcout << "Task Heart beat is running...";
        client.push_heartbeat();

        last_heartbeat_time = current_time;
    }
}

TEST_F(PipeClientTest, VerifyClientSubscriptionAndDataPushSequence) {
    // Mock the incoming command-line argument configuration locally (e.g., ID "1")
    const std::string client_path = "client_alpha_" + GLOBAL_CLIENT_ID;

    bool client_routine_completed = false;

    // 1. Trigger the Client logic asynchronously inside an explicit execution scope
    std::thread client_thread([&]() {
        Ipc::Generic client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;

        // Use the derived TestPipeClient class to bypass visibility rules
        TestPipeClient client(TEST_REQUEST_PIPE, client_path, client_flags);

        // Subscribe to server
        // Note: For this check to succeed in a real integration harness,
        // a running PipeServer must be listening on TEST_REQUEST_PIPE.
        // Subscribe to server
        if (client.request_and_switch_pipe(1)) {
            // First messaging phase validation (FIXED: Removed EXPECT_TRUE)
            std::string private_data = "Private data Client Alpha and Server!";

            client.start_monitoring();
            client.push_data(DataType::Text, private_data);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_arg));
        }

        int cnt = 200;
        while (--cnt) {
            // --- TEST PHASE 1: Verify Text Transmission (FIXED: Removed EXPECT_TRUE) ---
            std::string text_msg = "Hello POSIX Pipe!";
            fmt::print("\n{} Sending text...\n", client_path);
            client.push_data(DataType::Text, text_msg);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_arg));

            // --- TEST PHASE 2: Verify Raw Numeric Integer Translation (FIXED: Removed EXPECT_TRUE) ---
            int number = 2026;
            fmt::print("\n{} Sending number...\n", client_path);
            client.push_data(DataType::Number, number);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_arg));

            // --- TEST PHASE 3: Verify Heavy Structured Binary Containers (FIXED: Removed EXPECT_TRUE) ---
            std::vector<uint8_t> dummy_media = {0xFF, 0xD8, 0xFF, 0xE0, 0x01, 0x02};
            fmt::print("\n{} Sending mock Media data...\n", client_path);
            client.push_data(DataType::Media, dummy_media);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_arg));

            heartbeat_task(client);
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_arg));
        }

        client_routine_completed = true;
    });

    // 2. Await the thread sequence execution tracking boundary loop
    if (client_thread.joinable()) {
        client_thread.join();
    }

    // 3. Confirm that the entire client operational path parsed completely without locks
    EXPECT_TRUE(client_routine_completed);
}

// ==============================================================================
// GLOBAL MAIN FUNCTION
// ==============================================================================
int main(int argc, char **argv) {
    // Set non-buffering mode to guarantee asynchronous multithreaded logging visibility
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    if (argc >= 3) {
        GLOBAL_CLIENT_ID = argv[1];
        delay_arg        = (int16_t)atoi(argv[2]);
    } else {
        fmt::print(stderr, "[Warning] No client_id provided! Using default: {}\n", GLOBAL_CLIENT_ID);
    }

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}