#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <string>
#include <vector>

// Include your system architecture here
#include "universal_socket.h"
using namespace HarisLinux;

class UniversalSocketTest : public ::testing::Test {
 protected:
    // Define clean filesystem path boundaries for local IPC testing
    const std::string server_path = "/tmp/haris_server_test.sock";
    const std::string client_path = "/tmp/haris_client_test.sock";

    void SetUp() override {
        // Enforce deterministic environment by wiping stale socket nodes before any execution
        ::unlink(server_path.c_str());
        ::unlink(client_path.c_str());
    }

    void TearDown() override {
        // Prevent dangling socket file nodes on host filesystem post-test
        ::unlink(server_path.c_str());
        ::unlink(client_path.c_str());
    }
};

// ============================================================================
// TEST CASE 1: Connected Unix Datagram Transmission Protocol Verification
// ============================================================================
TEST_F(UniversalSocketTest, DgramPathPolicy_EndToEnd_String_Transmission) {
    // Compile asymmetric context maps for connected DGRAM socket bindings
    SocketDgramPathContext server_ctx;
    server_ctx.local_path  = server_path;
    server_ctx.target_path = client_path;

    SocketDgramPathContext client_ctx;
    client_ctx.local_path  = client_path;
    client_ctx.target_path = server_path;

    // Instantiate isolated pipeline nodes using Policy-Based compile-time selection
    UniversalSocket<void, SocketDgramPathPolicy> server_node(server_ctx);
    UniversalSocket<void, SocketDgramPathPolicy> client_node(client_ctx);

    const std::string sample_data = "Atomic vector data payload over Connected Unix Dgram!";
    const uint32_t    target_seq  = 101;
    const DataType    target_type = DataType::Text;  // Replace with your exact enum variant

    // Execute atomic send operations across stack frames (Zero-Copy input validation)
    bool send_status = client_node.send_packet(target_type, sample_data, target_seq);
    ASSERT_TRUE(send_status) << "Sender failed to execute writev processing corridor.";

    // Allocate receiving buffers on stack / heap frames
    PacketHeader         received_header{};
    std::vector<uint8_t> received_payload;

    // Execute atomic read operation (Triggers MSG_PEEK + recvmsg scatter-gather pipeline)
    bool recv_status = server_node.receive_packet(received_header, received_payload);
    ASSERT_TRUE(recv_status) << "Receiver dropped frame during scatter-gather recovery.";

    // Validate structural framework compliance rules
    EXPECT_EQ(received_header.type, target_type);
    EXPECT_EQ(received_header.sequence_id, target_seq);
    EXPECT_EQ(received_header.payload_size, sample_data.size());

    // Recover string layout directly out of raw payload byte vector buffers
    std::string decoded_output(reinterpret_cast<char*>(received_payload.data()), received_payload.size());
    EXPECT_EQ(decoded_output, sample_data);
}

// ============================================================================
// TEST CASE 2: Native C-Struct Data Type Compilation and Safety Check
// ============================================================================
TEST_F(UniversalSocketTest, DgramPathPolicy_POD_Struct_Transmission) {
    HarisLinux::SocketDgramPathContext server_ctx;
    server_ctx.local_path  = server_path;
    server_ctx.target_path = client_path;

    HarisLinux::SocketDgramPathContext client_ctx;
    client_ctx.local_path  = client_path;
    client_ctx.target_path = server_path;

    UniversalSocket<void, SocketDgramPathPolicy> server_node(server_ctx);
    UniversalSocket<void, SocketDgramPathPolicy> client_node(client_ctx);

    // Instantiate standard fixed-size payload layout to hit the 'is_arithmetic / custom' compiler branch
    IPCRequestPayload request;
    std::memset(request.client_id, 0, sizeof(request.client_id));
    std::strncpy(request.client_id, "client_pipeline_node_A", sizeof(request.client_id) - 1);
    request.command = 1;
    struct iovec iov_struct;
    iov_struct.iov_base = &request;
    iov_struct.iov_len  = sizeof(IPCRequestPayload);
    bool send_status    = client_node.send_packet(DataType::Command, request, 202);
    ASSERT_TRUE(send_status);

    PacketHeader         received_header{};
    std::vector<uint8_t> received_payload;

    bool recv_status = server_node.receive_packet(received_header, received_payload);
    ASSERT_TRUE(recv_status);

    EXPECT_EQ(received_header.payload_size, sizeof(IPCRequestPayload));

    // Safely cast recovered data segment back to localized structural pointer
    ASSERT_EQ(received_payload.size(), sizeof(IPCRequestPayload));
    auto* inbound_payload = reinterpret_cast<IPCRequestPayload*>(received_payload.data());

    EXPECT_STREQ(inbound_payload->client_id, request.client_id);

    EXPECT_EQ(std::string_view(inbound_payload->client_id, sizeof(inbound_payload->client_id)),
              std::string_view(request.client_id, sizeof(request.client_id)));
    EXPECT_EQ(inbound_payload->command, request.command);
}

// ============================================================================
// TEST CASE 3: Empty Payload Bound Processing Verification
// ============================================================================
TEST_F(UniversalSocketTest, DgramPathPolicy_Zero_Length_Payload_Signaling) {
    HarisLinux::SocketDgramPathContext server_ctx;
    server_ctx.local_path  = server_path;
    server_ctx.target_path = client_path;

    HarisLinux::SocketDgramPathContext client_ctx;
    client_ctx.local_path  = client_path;
    client_ctx.target_path = server_path;

    UniversalSocket<void, SocketDgramPathPolicy> server_node(server_ctx);
    UniversalSocket<void, SocketDgramPathPolicy> client_node(client_ctx);

    // Testing dynamic optimization branch when iov_len = 1 (Header only)
    std::string empty_data = "";

    bool send_status = client_node.send_packet(DataType::Text, empty_data, 777);
    ASSERT_TRUE(send_status);

    PacketHeader         received_header{};
    std::vector<uint8_t> received_payload;

    bool recv_status = server_node.receive_packet(received_header, received_payload);
    ASSERT_TRUE(recv_status);

    EXPECT_EQ(received_header.payload_size, 0);
    EXPECT_TRUE(received_payload.empty());
}