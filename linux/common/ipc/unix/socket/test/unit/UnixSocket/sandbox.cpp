#include "unix_socket.h"

#include <gtest/gtest.h>

using namespace HarisLinux;

// Include other matching headers like packet_header.h if required by your layout
// =========================================================================
// TEST SUITE 1: COMPILE-TIME METRICS & INHERITANCE TREE VALIDATION
// =========================================================================
TEST(UnixSocketCompileTest, InheritanceLayoutConstraints) {
    // Verify type-trait conditional inheritance maps safely at compile time
    static_assert(std::is_base_of_v<HarisLinux::StreamSender, HarisLinux::UnixSocket<Ipc::Server>>,
                  "Error: Server instance must inherit from StreamSender base class!");

    static_assert(!std::is_base_of_v<HarisLinux::DgramSender, HarisLinux::UnixSocket<Ipc::Server>>,
                  "Error: Server instance should NOT allocate memory for DgramSender!");

    SUCCEED() << "Compile-time layout constraints validated smoothly.";
}

// =========================================================================
// TEST SUITE 2: COMPLETE STREAM (TCP/PIPE) SEND & RECEIVE LOOP
// =========================================================================
TEST(UnixSocketIntegrationTest, StreamSendReceiveLoop) {
    Ipc::Generic<Ipc::Server> server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;
    uint32_t                  sample_seq   = 777;

    HarisLinux::UnixSocket<Ipc::Server, Ipc::StreamTag> stream_socket(AF_UNIX, SOCK_STREAM, server_flags);

    int pipe_fds[2];
    ASSERT_NE(::pipe(pipe_fds), -1) << "Failed to allocate OS pipe markers";

    int stream_write_fd = pipe_fds[1];
    int stream_read_fd  = pipe_fds[0];

    std::string stream_tx_data = "HarisStreamDataPayload";

    // 1. Verify packet transmission over Stream
    bool stream_send_ok =
        stream_socket.send_packet(stream_write_fd, HarisLinux::DataType::Text, stream_tx_data, sample_seq);
    EXPECT_TRUE(stream_send_ok);

    HarisLinux::PacketHeader stream_header{};
    std::vector<uint8_t>     stream_rx_payload;

    // 2. Verify packet reception over Stream
    bool stream_recv_ok = stream_socket.receive_packet(stream_read_fd, stream_header, stream_rx_payload);
    ASSERT_TRUE(stream_recv_ok);

    // 3. Validate data integrity
    std::string stream_rx_data(stream_rx_payload.begin(), stream_rx_payload.end());
    EXPECT_EQ(stream_header.sequence_id, sample_seq);
    EXPECT_EQ(stream_rx_data, stream_tx_data);

    // Secure cleanup of descriptor pipelines
    ::close(stream_write_fd);
    ::close(stream_read_fd);
}

// =========================================================================
// TEST SUITE 3: COMPLETE DATAGRAM (UDP/SOCKET) SEND & RECEIVE LOOP
// =========================================================================
TEST(UnixSocketIntegrationTest, DatagramSendReceiveLoop) {
    Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;
    uint32_t                  sample_seq   = 777;

    const std::string server_path = "/tmp/UnixSocket.sock";

    // Ensure old tracking remnants are removed from the filesystem boundary
    ::unlink(server_path.c_str());

    // 1. Provision two standard, raw UNIX datagram socket descriptors
    int dgram_server_fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
    int dgram_client_fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
    ASSERT_NE(dgram_server_fd, -1) << "Failed to provision server socket context";
    ASSERT_NE(dgram_client_fd, -1) << "Failed to provision client socket context";

    // 2. Bind the server socket to its physical filesystem path descriptor
    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    std::strncpy(server_addr.sun_path, server_path.c_str(), sizeof(server_addr.sun_path) - 1);

    int bind_res = ::bind(dgram_server_fd, reinterpret_cast<const sockaddr *>(&server_addr), sizeof(server_addr));
    if (bind_res == -1) {
        ::close(dgram_server_fd);
        ::close(dgram_client_fd);
        FAIL() << "Failed to execute server channel bind sequencing";
    }

    // 3. Connect the client socket context explicitly to the target server address loop
    int connect_res = ::connect(dgram_client_fd, reinterpret_cast<const sockaddr *>(&server_addr), sizeof(server_addr));
    if (connect_res == -1) {
        ::close(dgram_server_fd);
        ::close(dgram_client_fd);
        ::unlink(server_path.c_str());
        FAIL() << "Failed to connect client pipe to target server destination";
    }

    // 4. Initialize class framework instance
    HarisLinux::UnixSocket<Ipc::Client, Ipc::DgramTag> dgram_socket(AF_UNIX, SOCK_DGRAM, client_flags);
    std::string                                        dgram_tx_data = "HarisDgramDataPayload";

    // 5. Transmit through the connected client descriptor
    bool dgram_send_ok =
        dgram_socket.send_packet(dgram_client_fd, HarisLinux::DataType::Text, dgram_tx_data, sample_seq + 1);
    EXPECT_TRUE(dgram_send_ok);

    HarisLinux::PacketHeader dgram_header{};
    std::vector<uint8_t>     dgram_rx_payload;

    // 6. Extract the packet from the server's descriptor where the frame was routed
    bool dgram_recv_ok = dgram_socket.receive_packet(dgram_server_fd, dgram_header, dgram_rx_payload);
    ASSERT_TRUE(dgram_recv_ok);

    // 7. Verify network data integrity
    std::string dgram_rx_data(dgram_rx_payload.begin(), dgram_rx_payload.end());
    EXPECT_EQ(dgram_header.sequence_id, sample_seq + 1);
    EXPECT_EQ(dgram_rx_data, dgram_tx_data);

    // 8. Secure cleanup pipelines to prevent unlinked resource leaks
    ::close(dgram_client_fd);
    ::close(dgram_server_fd);
    ::unlink(server_path.c_str());
}

// Standard Google Test main runner initialization
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}