#include "ipc_sender.h"
#include "ipc_receiver.h"

#include <gtest/gtest.h>

using namespace HarisLinux;

const std::string SOCK_PATH = "/tmp/test_crtp_dgram.sock";

struct DeviceConfig {
    uint32_t               device_id;
    float                  sample_rate;
    const std::string_view status_code;
};

// ==============================================================================
// TEST DERIVATION WRAPPERS (Spy Pattern via 'using')
// ==============================================================================
class TestStreamReceiver : public HarisLinux::StreamReceiver {
 public:
    using HarisLinux::StreamReceiver::receive;
    using HarisLinux::StreamReceiver::StreamReceiver;
};

class TestStreamSender : public HarisLinux::StreamSender {
 public:
    using HarisLinux::StreamSender::send;
    using HarisLinux::StreamSender::StreamSender;
};

class TestDgramReceiver : public HarisLinux::DgramReceiver {
 public:
    using HarisLinux::DgramReceiver::DgramReceiver;
    using HarisLinux::DgramReceiver::receive;
};

class TestDgramSender : public HarisLinux::DgramSender {
 public:
    using HarisLinux::DgramSender::DgramSender;
    using HarisLinux::DgramSender::send;
};

// ==============================================================================
// GOOGLETEST FIXTURE
// ==============================================================================
class IPCSystemTest : public ::testing::Test {
 protected:
    int   pipe_fds[2] = {-1, -1};
    int   dgram_fd    = -1;
    pid_t pid         = -1;

    void SetUp() override {
        // 1. Create Pipe
        ASSERT_GE(pipe(pipe_fds), 0) << "Failed to create Pipe";

        // 2. Setup UNIX Datagram Socket
        unlink(SOCK_PATH.c_str());
        dgram_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
        ASSERT_GE(dgram_fd, 0) << "Failed to create Socket";

        sockaddr_un local_addr{};
        std::memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sun_family = AF_UNIX;
        std::strncpy(local_addr.sun_path, SOCK_PATH.c_str(), sizeof(local_addr.sun_path) - 1);

        ASSERT_GE(bind(dgram_fd, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)), 0)
            << "Bind socket failed";
    }

    void TearDown() override {
        if (pipe_fds[0] >= 0) close(pipe_fds[0]);
        if (pipe_fds[1] >= 0) close(pipe_fds[1]);
        if (dgram_fd >= 0) close(dgram_fd);

        if (pid > 0) {
            int   status;
            pid_t return_pid = waitpid(pid, &status, WNOHANG);
            if (return_pid == 0) {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
            }
        }
        unlink(SOCK_PATH.c_str());
    }

    void run_sender_logic() {
        usleep(10000);  // Tiny wait to let receiver warm up

        // 1. Send via PIPE Stream using Derived Test Class
        TestStreamSender pipe_sender(pipe_fds[1]);
        uint32_t         test_int = 756799112;
        pipe_sender.send(DataType::Number, test_int, 101);

        std::string test_str = "IPC system is optimized by CRTP";
        pipe_sender.send(DataType::Text, test_str, 102);

        // 2. Send via UNIX Datagram using Derived Test Class
        TestDgramSender dgram_sender(dgram_fd, SOCK_PATH);

        std::vector<uint8_t> mock_image(2048, 0x00);
        mock_image[0]                     = 0xAA;
        mock_image[mock_image.size() - 1] = 0xBB;
        dgram_sender.send(DataType::Media, mock_image, 201);

        DeviceConfig         config{9999, 44100.0f, "Hello"};
        std::vector<uint8_t> struct_buffer(sizeof(DeviceConfig));
        std::memcpy(struct_buffer.data(), &config, sizeof(DeviceConfig));
        dgram_sender.send(DataType::Custom, struct_buffer, 202);

        _exit(0);  // Clean fail-safe sub-process exit
    }
};

// ==============================================================================
// VERIFICATION TEST CASES (Executed by Parent / Receiver Process)
// ==============================================================================

TEST_F(IPCSystemTest, VerifyCombinedPipeAndDatagramE2E) {
    // Fork processes inside the test framework runner loop
    pid = fork();
    ASSERT_GE(pid, 0) << "Fork failed";

    if (pid == 0) {
        close(pipe_fds[0]);  // Close unused read end in child
        run_sender_logic();
    }

    close(pipe_fds[1]);  // Close unused write end in parent

    PacketHeader         header;
    std::vector<uint8_t> payload;
    payload.reserve(4096);

    // -------------------------------------------------------------------------
    // 1. Verify PIPE (StreamReceiver Paths)
    // -------------------------------------------------------------------------
    TestStreamReceiver stream_receiver(pipe_fds[0]);

    // Test case 1.1: Data type is Number
    ASSERT_TRUE(stream_receiver.receive(header, payload));
    EXPECT_EQ(header.type, DataType::Number);
    EXPECT_EQ(*reinterpret_cast<uint32_t*>(payload.data()), 756799112u);

    // Test case 1.2: Data type is Text
    ASSERT_TRUE(stream_receiver.receive(header, payload));
    EXPECT_EQ(header.type, DataType::Text);
    std::string received_str(reinterpret_cast<char*>(payload.data()), header.payload_size);
    EXPECT_EQ(received_str, "IPC system is optimized by CRTP");

    // -------------------------------------------------------------------------
    // 2. Verify UNIX Socket Datagram (DgramReceiver Paths)
    // -------------------------------------------------------------------------
    TestDgramReceiver dgram_receiver(dgram_fd);

    // Test case 2.1: Data type is Media
    ASSERT_TRUE(dgram_receiver.receive(header, payload));
    EXPECT_EQ(header.type, DataType::Media);
    ASSERT_GE(payload.size(), 2u);
    EXPECT_EQ(payload[0], 0xAA);
    EXPECT_EQ(payload[payload.size() - 1], 0xBB);

    // Test case 2.2: Data type is Custom (Struct Configuration Mapping)
    ASSERT_TRUE(dgram_receiver.receive(header, payload));
    EXPECT_EQ(header.type, DataType::Custom);

    DeviceConfig* config = reinterpret_cast<DeviceConfig*>(payload.data());
    EXPECT_EQ(config->device_id, 9999u);
    EXPECT_FLOAT_EQ(config->sample_rate, 44100.0f);
}

// Global Main entry point to invoke GoogleTest engine loops
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}