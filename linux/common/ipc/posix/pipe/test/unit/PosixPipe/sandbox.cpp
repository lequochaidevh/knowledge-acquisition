#include "posix_pipe.h"
#include <gtest/gtest.h>

using namespace HarisLinux;

// Custom Data Struct for validating raw memory transmissions
struct SensorPayload {
    uint64_t timestamp;
    double   temperature;
    uint32_t error_code;
};

const std::string MAIN_PIPE_PATH = "/tmp/haris_pipe_main";

// ==============================================================================
// TEST DERIVATION LAYER (Spy Pattern)
// Exposes protected methods of the template class to public visibility for testing
// ==============================================================================
template <typename Modes>
class TestPosixPipe : public HarisLinux::PosixPipe<Modes> {
 public:
    // Explicitly define and forward the constructor to the base protected constructor
    TestPosixPipe(const std::string& path, HarisLinux::Ipc::Generic<Modes> modes)
        : HarisLinux::PosixPipe<Modes>(path, modes) {}

    // Pull the protected modifier methods into public scope
    UniqueFileDescriptor test_set_read_fd(int read_fd) {
        // if (read_fd < 0) return;

        /* Step 1: store main write fd before */
        UniqueFileDescriptor store_main_fd = std::move(StreamReceiver::_unique_fd);

        /* Step 2: Covert raw fd to smart fd */
        UniqueFileDescriptor unique_fd(read_fd, FileType::Pipe);
        StreamReceiver::_unique_fd = std::move(unique_fd);

        /* Step 3: store smart fd */
        return std::move(store_main_fd);
    }
    bool test_set_write_fd(int write_fd) {
        StreamSender::_fd_list.set_active_fd(1);
        /* Covert raw fd to smart fd */
        UniqueFileDescriptor unique_fd(write_fd, FileType::Pipe);
        StreamSender::_fd_list.reset_active_fd(std::move(unique_fd));

        return true;
    }

    // Pull the protected data transmission methods into public scope
    using HarisLinux::PosixPipe<Modes>::send_packet;
    using HarisLinux::PosixPipe<Modes>::receive_packet;
};

// ==============================================================================
// GOOGLETEST FIXTURE
// ==============================================================================
class IPCPosixPipeTest : public ::testing::Test {
 protected:
    pid_t child_pid = -1;
    int   read_fd   = -1;

    void SetUp() override {
        // Always clean up stale artifacts from old test crashes before beginning
        unlink(MAIN_PIPE_PATH.c_str());
        ASSERT_GE(mkfifo(MAIN_PIPE_PATH.c_str(), 0666), 0) << "Failed to create named pipe (mkfifo)";
    }

    void TearDown() override {
        // Safely close the parent's file descriptor if it remains active
        if (read_fd >= 0) {
            close(read_fd);
            read_fd = -1;
        }

        // Handle process resource reclamation to prevent terminal freezes and deadlocks
        if (child_pid > 0) {
            int status;
            // Check instantly if the child process has exited without blocking (WNOHANG)
            pid_t return_pid = waitpid(child_pid, &status, WNOHANG);

            if (return_pid == 0) {
                // Forcefully terminate the child process if it is caught hanging or deadlocked
                kill(child_pid, SIGKILL);
                waitpid(child_pid, &status, 0);  // Reclaim resources immediately
            }
        }
        unlink(MAIN_PIPE_PATH.c_str());
    }

    // Encapsulated Sender logic executed exclusively inside the sub-process
    void run_child_sender_logic() {
        // Open the write channel (Blocks natively until the parent opens O_RDONLY)
        int write_fd = open(MAIN_PIPE_PATH.c_str(), O_WRONLY);
        if (write_fd < 0) {
            std::exit(1);
        }

        Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;

        // Use the public wrapper class instead of the raw protected class
        TestPosixPipe<Ipc::Client> pipe_sender(MAIN_PIPE_PATH, client_flags);
        pipe_sender.test_set_write_fd(write_fd);

        // Test case 1: Dispatch an integer primitive
        int system_code = 2026;
        pipe_sender.send_packet(pipe_sender.get_write_fd(), DataType::Number, system_code, 1);

        // Test case 2: Dispatch a standard string container
        std::string alert_msg = "HarisLinux Pipe CRTP Engine Running!";
        pipe_sender.send_packet(pipe_sender.get_write_fd(), DataType::Text, alert_msg, 2);

        // Test case 3: Dispatch a plain old data struct via a raw buffer
        SensorPayload        sensor_data{123456789, 36.5, 0};
        std::vector<uint8_t> struct_buffer(sizeof(SensorPayload));
        std::memcpy(struct_buffer.data(), &sensor_data, sizeof(SensorPayload));
        pipe_sender.send_packet(pipe_sender.get_write_fd(), DataType::Custom, struct_buffer, 3);

        close(write_fd);
        std::exit(0);  // Explicitly terminate to prevent the child from entering the test harness loop
    }
};

// ==============================================================================
// THE ACTUAL TEST CASE
// ==============================================================================
TEST_F(IPCPosixPipeTest, VerifyNamedPipeTransmissionE2E) {
    // Perform process fork directly within the test body for ideal GoogleTest tracing
    child_pid = fork();
    ASSERT_GE(child_pid, 0) << "Process bifurcation failed (fork)";

    if (child_pid == 0) {
        run_child_sender_logic();
    }

    // --- PARENT PROCESS (RECEIVER) LOGIC ---
    read_fd = open(MAIN_PIPE_PATH.c_str(), O_RDONLY);
    ASSERT_GE(read_fd, 0) << "Parent process failed to open read_fd";

    // Initialize the main reading pipeline configuration using the Test Wrapper
    Ipc::Generic<Ipc::Server>  server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;
    TestPosixPipe<Ipc::Server> pipe_receiver(MAIN_PIPE_PATH, server_flags);
    pipe_receiver.test_set_read_fd(read_fd);

    PacketHeader         header;
    std::vector<uint8_t> payload;

    // Phase 1 Validation: Verify incoming numeric type data streams
    ASSERT_TRUE(pipe_receiver.receive_packet(pipe_receiver.get_read_fd(), header, payload))
        << "Failed to receive numeric payload";
    EXPECT_EQ(header.type, DataType::Number);
    EXPECT_EQ(*reinterpret_cast<int*>(payload.data()), 2026);

    // Phase 2 Validation: Verify incoming string type data streams
    ASSERT_TRUE(pipe_receiver.receive_packet(pipe_receiver.get_read_fd(), header, payload))
        << "Failed to receive textual payload";
    EXPECT_EQ(header.type, DataType::Text);
    std::string text(reinterpret_cast<char*>(payload.data()), header.payload_size);
    EXPECT_EQ(text, "HarisLinux Pipe CRTP Engine Running!");

    // Phase 3 Validation: Verify incoming structured blob type data streams
    ASSERT_TRUE(pipe_receiver.receive_packet(pipe_receiver.get_read_fd(), header, payload))
        << "Failed to receive custom structured payload";
    EXPECT_EQ(header.type, DataType::Custom);
    SensorPayload* received_struct = reinterpret_cast<SensorPayload*>(payload.data());
    EXPECT_DOUBLE_EQ(received_struct->temperature, 36.5);  // Safe precision floating point check
    EXPECT_EQ(received_struct->error_code, 0u);
}

// ==============================================================================
// GLOBAL MAIN FUNCTION
// ==============================================================================
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}