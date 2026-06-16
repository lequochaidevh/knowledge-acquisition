#include "ipc_sender.h"
#include "ipc_receiver.h"

#include <gtest/gtest.h>

using namespace HarisLinux;

const std::string SOCK_PATH = "/tmp/test_crtp_dgram.sock";

struct DeviceConfig {
    uint32_t               device_id;
    float                  sample_rate;
    const std::string_view status_code;  // Note: sending string_view over IPC contains a pointer,
                                         // ensure sender/receiver share exact memory or use fixed-size chars.
};

enum class ModeIPCTest { Stream, Dgram };

// --- Wrapper Wrappers kept intact ---
class IPCReceiverTest {
 private:
    ModeIPCTest    _mode;
    StreamReceiver _stream_receiver;
    DgramReceiver  _dgram_receiver;

 public:
    IPCReceiverTest(ModeIPCTest m, int pipe_read_fd)
        : _mode(m), _stream_receiver(pipe_read_fd), _dgram_receiver(pipe_read_fd) {}

    bool receive_fw(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
        if (_mode == ModeIPCTest::Stream) return _stream_receiver.receive(out_header, out_payload);
        return _dgram_receiver.receive(out_header, out_payload);
    }
};

class IPCSenderTest {
 private:
    ModeIPCTest  _mode;
    StreamSender _stream_sender;
    DgramSender  _dgram_sender;

 public:
    IPCSenderTest(ModeIPCTest m, int pipe_read_fd, const std::string& target_path)
        : _mode(m), _stream_sender(pipe_read_fd), _dgram_sender(pipe_read_fd, target_path) {}

    template <typename T>
    bool send_fw(DataType data_type, const T& data, const uint32_t& seq = 0) {
        if (_mode == ModeIPCTest::Stream) return _stream_sender.send(data_type, data, seq);
        return _dgram_sender.send(data_type, data, seq);
    }
};

// -------------------------------------------------------------------------
// GOOGLETEST FIXTURE
// -------------------------------------------------------------------------
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

        // 3. Fork Processes
        pid = fork();
        ASSERT_GE(pid, 0) << "Fork failed";

        if (pid == 0) {
            // CHILD PROCESS (Sender) Logic Execution
            close(pipe_fds[0]);  // Close unused read end
            run_sender_logic();
            close(pipe_fds[1]);
            close(dgram_fd);
            std::exit(0);  // Safely exit the child process so GoogleTest framework doesn't duplicate loops
        } else {
            // PARENT PROCESS (Receiver) Configuration
            close(pipe_fds[1]);  // Close unused write end
        }
    }

    void TearDown() override {
        if (pid > 0) {
            close(pipe_fds[0]);
            close(dgram_fd);
            waitpid(pid, nullptr, 0);  // Reclaim child process resources
            unlink(SOCK_PATH.c_str());
        }
    }

 private:
    void run_sender_logic() {
        sleep(1);  // Wait briefly for receiver to be active

        // 1. PIPE Stream
        IPCSenderTest pipe_sender(ModeIPCTest::Stream, pipe_fds[1], SOCK_PATH);
        uint32_t      test_int = 756799112;
        pipe_sender.send_fw<uint32_t>(DataType::Number, test_int, 101);

        std::string test_str = "IPC system is optimized by CRTP";
        pipe_sender.send_fw(DataType::Text, test_str, 102);

        // 2. UNIX Datagram
        IPCSenderTest dgram_sender(ModeIPCTest::Dgram, dgram_fd, SOCK_PATH);

        std::vector<uint8_t> mock_image(2048, 0x00);
        mock_image[0]                     = 0xAA;
        mock_image[mock_image.size() - 1] = 0xBB;
        dgram_sender.send_fw(DataType::Media, mock_image, 201);

        DeviceConfig         config{9999, 44100.0f, "Hello"};
        std::vector<uint8_t> struct_buffer(sizeof(DeviceConfig));
        std::memcpy(struct_buffer.data(), &config, sizeof(DeviceConfig));
        dgram_sender.send_fw(DataType::Custom, struct_buffer, 202);
    }
};

// -------------------------------------------------------------------------
// GOOGLETEST UNIT TESTS (Executed by the Parent Receiver Process)
// -------------------------------------------------------------------------

TEST_F(IPCSystemTest, PipeStreamVerification) {
    PacketHeader         header;
    std::vector<uint8_t> payload;
    payload.reserve(4096);

    IPCReceiverTest stream_receiver(ModeIPCTest::Stream, pipe_fds[0]);

    // Test case 1.1 - Data type is Number
    ASSERT_TRUE(stream_receiver.receive_fw(header, payload));
    EXPECT_EQ(header.type, DataType::Number);
    int received_val = *reinterpret_cast<int*>(payload.data());
    EXPECT_EQ(received_val, 756799112);

    // Test case 1.2 - Data type is Text
    ASSERT_TRUE(stream_receiver.receive_fw(header, payload));
    EXPECT_EQ(header.type, DataType::Text);
    std::string received_str(reinterpret_cast<char*>(payload.data()), header.payload_size);
    EXPECT_EQ(received_str, "IPC system is optimized by CRTP");
}

TEST_F(IPCSystemTest, UnixDatagramVerification) {
    PacketHeader         header;
    std::vector<uint8_t> payload;
    payload.reserve(4096);

    IPCReceiverTest dgram_receiver(ModeIPCTest::Dgram, dgram_fd);

    // Skip the 2 pipe outputs since we are only verifying Datagram paths here
    IPCReceiverTest bypass_stream(ModeIPCTest::Stream, pipe_fds[0]);
    bypass_stream.receive_fw(header, payload);
    bypass_stream.receive_fw(header, payload);

    // Test case 2.1 - Data type is Media
    ASSERT_TRUE(dgram_receiver.receive_fw(header, payload));
    EXPECT_EQ(header.type, DataType::Media);
    ASSERT_GE(payload.size(), 2u);
    EXPECT_EQ(payload[0], 0xAA);
    EXPECT_EQ(payload[payload.size() - 1], 0xBB);

    // Test case 2.2 - Data type is Custom (Struct)
    ASSERT_TRUE(dgram_receiver.receive_fw(header, payload));
    EXPECT_EQ(header.type, DataType::Custom);

    DeviceConfig* config = reinterpret_cast<DeviceConfig*>(payload.data());
    EXPECT_EQ(config->device_id, 9999);
    EXPECT_FLOAT_EQ(config->sample_rate, 44100.0f);
}

// Merge to once test
TEST_F(IPCSystemTest, VerifyBothPipeAndDatagram) {
    PacketHeader         header;
    std::vector<uint8_t> payload;
    payload.reserve(4096);

    // ==========================================
    // PHASE 1: Test Pipe Stream
    // ==========================================
    IPCReceiverTest stream_receiver(ModeIPCTest::Stream, pipe_fds[0]);

    // Test case 1.1 - Data type is Number
    ASSERT_TRUE(stream_receiver.receive_fw(header, payload));
    EXPECT_EQ(header.type, DataType::Number);
    int received_val = *reinterpret_cast<int*>(payload.data());
    EXPECT_EQ(received_val, 756799112);

    // Test case 1.2 - Data type is Text
    ASSERT_TRUE(stream_receiver.receive_fw(header, payload));
    EXPECT_EQ(header.type, DataType::Text);
    std::string received_str(reinterpret_cast<char*>(payload.data()), header.payload_size);
    EXPECT_EQ(received_str, "IPC system is optimized by CRTP");

    // ==========================================
    // PHASE 2: Test UNIX Datagram
    // ==========================================
    IPCReceiverTest dgram_receiver(ModeIPCTest::Dgram, dgram_fd);

    // Test case 2.1 - Data type is Media
    ASSERT_TRUE(dgram_receiver.receive_fw(header, payload));
    EXPECT_EQ(header.type, DataType::Media);
    ASSERT_GE(payload.size(), 2u);
    EXPECT_EQ(payload[0], 0xAA);
    EXPECT_EQ(payload[payload.size() - 1], 0xBB);

    // Test case 2.2 - Data type is Custom (Struct)
    ASSERT_TRUE(dgram_receiver.receive_fw(header, payload));
    EXPECT_EQ(header.type, DataType::Custom);

    DeviceConfig* config = reinterpret_cast<DeviceConfig*>(payload.data());
    EXPECT_EQ(config->device_id, 9999);
    EXPECT_FLOAT_EQ(config->sample_rate, 44100.0f);
}

// Global Main entry point to invoke all tests
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}