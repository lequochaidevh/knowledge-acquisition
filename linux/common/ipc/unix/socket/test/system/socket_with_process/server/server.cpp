#include "socket_server.h"
#include "socket_client.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

using namespace HarisLinux;

// =========================================================================
// TEST SUITE 1: SYMMETRICAL UNIX DOMAIN DATAGRAM (UDP/SOCK_DGRAM) PIPELINE
// =========================================================================
TEST(SocketSystemIntegrationTest, SymmetricalDatagramTransmissionLoop) {
    // 💡 FIX: Allocate an interconnected loopback kernel socket pair descriptor match!
    // This allows FileType::Pipe to read data off a DGRAM descriptor smoothly without kernel boundary drops.
    int sv[2];
    ASSERT_NE(::socketpair(AF_UNIX, SOCK_DGRAM, 0, sv), -1) << "Failed to allocate symmetrical socket pairs.";

    int client_dgram_fd = sv[0];
    int server_dgram_fd = sv[1];

    Ipc::Generic<Ipc::Server> server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;
    Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;

    // Initialize abstractions using matching IIpc naming convention
    HarisLinux::SocketServer<IIpc::DgramTag> server(AF_UNIX, SOCK_DGRAM, server_flags);
    HarisLinux::SocketClient<IIpc::DgramTag> client(AF_UNIX, SOCK_DGRAM, client_flags);

    // Apply safety receive timeouts so automated test runners never freeze
    struct timeval tv {
        0, 50000
    };  // 50ms window
    ::setsockopt(client_dgram_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(server_dgram_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // 💡 CLIENT TX -> SERVER RX: Invoke public base parsing logic directly via the connected socket pairs
    std::string sample_payload = "HarisSymmetricalDgramE2E";
    uint32_t    test_seq       = 555;

    // Fire the text packet into the symmetrical loopback
    bool dgram_send_ok = client.send_packet(client_dgram_fd, DataType::Text, sample_payload, test_seq);
    EXPECT_TRUE(dgram_send_ok) << "Base class send_packet rejected loopback transmission.";

    PacketHeader         server_rx_header{};
    std::vector<uint8_t> server_rx_payload;

    // Extract the packet via the server's end of the connected socket pair
    bool dgram_recv_ok = server.receive_packet(server_dgram_fd, server_rx_header, server_rx_payload);
    ASSERT_TRUE(dgram_recv_ok) << "Base class receive_packet failed to clear loopback payload buffer.";

    // Verify network data integrity matching your raw success profiles
    std::string validated_text(server_rx_payload.begin(), server_rx_payload.end());
    EXPECT_EQ(server_rx_header.type, DataType::Text);
    EXPECT_EQ(server_rx_header.sequence_id, test_seq);
    EXPECT_EQ(validated_text, sample_payload);

    // Securely close raw testing pipelines
    ::close(client_dgram_fd);
    ::close(server_dgram_fd);
}
// =========================================================================
// TEST SUITE 2: STANDARD UNIX DOMAIN STREAM (TCP/SOCK_STREAM) PIPELINE
// =========================================================================
TEST(SocketSystemIntegrationTest, ReliableStreamTransmissionLoop) {
    const std::string test_stream_path = "/tmp/uds_stream_integration.sock";

    // 1. Force clear stale tracking remnants from the file system boundary
    ::unlink(test_stream_path.c_str());

    Ipc::Generic<Ipc::Server> server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;
    Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;  // Default raw operations

    HarisLinux::SocketServer<IIpc::StreamTag> server(AF_UNIX, SOCK_STREAM, server_flags);
    HarisLinux::SocketClient<IIpc::StreamTag> client(AF_UNIX, SOCK_STREAM, client_flags);

    // 2. Drive the reliable connection sequence
    ASSERT_TRUE(server.start_server(test_stream_path)) << "Server failed to bind stream descriptor.";
    ASSERT_TRUE(client.connect_to_server(test_stream_path)) << "Client failed connection handshake to stream path.";
    ASSERT_TRUE(server.accept_connection()) << "Server failed to fully accept incoming link connection.";

    // 3. Set window boundaries for safety
    struct timeval tv {
        0, 50000
    };  // 50ms window
    ::setsockopt(client.get_socket_fd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ::setsockopt(server.get_socket_fd(), SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // 4. Drive structural transmission
    EXPECT_TRUE(client.push_data(DataType::Number, 8888)) << "Client dropped reliable stream push metrics.";

    PacketHeader         server_rx_header{};
    std::vector<uint8_t> server_rx_payload;
    ASSERT_TRUE(server.pull_data(server_rx_header, server_rx_payload)) << "Server stream pull execution stalled.";

    // 5. Verify message signatures match perfectly
    EXPECT_EQ(server_rx_header.type, DataType::Number);

    // 6. Tear down nodes cleanly
    ::unlink(test_stream_path.c_str());
}

// Global Google Test Runner Entry Point
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}