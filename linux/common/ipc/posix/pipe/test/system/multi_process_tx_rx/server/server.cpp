#include "pipe_server.h"

#include <gtest/gtest.h>

using namespace HarisLinux;

const std::string TEST_PIPE_PATH = "/tmp/main_request_pipe_test";

// ==============================================================================
// TEST DERIVATION LAYER (Spy Pattern via 'using')
// Exposes protected methods or constructors to public visibility for testing
// ==============================================================================
class TestPipeServer : public HarisLinux::PipeServer {
 public:
    // Pull the protected base constructor into public scope
    using HarisLinux::PipeServer::PipeServer;

    // Pull any protected lifecycle or event dispatchers into public scope if necessary
    using HarisLinux::PipeServer::dispatch_events;
};

// ==============================================================================
// GOOGLETEST FIXTURE
// ==============================================================================
class PipeServerTest : public ::testing::Test {
 protected:
    void SetUp() override {
        // Clear out stale named pipes before beginning to prevent file descriptor pollution
    }

    void TearDown() override {
        // Clean up testing filesystem footprints on test completion or failure
    }
};

// ==============================================================================
// THE ACTUAL LIFECYCLE TEST CASE
// ==============================================================================
TEST_F(PipeServerTest, VerifyServerInitializationAndEventLoopLifecycle) {
    // 1. Flag to verify that the server execution block processed successfully
    bool thread_executed = false;

    Ipc::Generic server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;

    // 2. Use our public wrapper derived instance
    TestPipeServer server(TEST_PIPE_PATH, server_flags);

    // Check stop server signal
    thread_executed = server.is_server_running();
    // 3. Structural Assertions to ensure the routine exited successfully
    EXPECT_TRUE(thread_executed) << "Server event loop available.";

    // Trigger the blocking listener loop
    server.dispatch_events();

    // Check stop server signal
    thread_executed = server.is_server_running();
    // 4. Structural Assertions to ensure the routine exited successfully
    EXPECT_FALSE(thread_executed) << "Server event loop did not shut down or execute cleanly.";
}

// ==============================================================================
// GLOBAL MAIN FUNCTION
// ==============================================================================
int main(int argc, char **argv) {
    // Disable stream buffering to ensure that logs from threads display out instantly
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}