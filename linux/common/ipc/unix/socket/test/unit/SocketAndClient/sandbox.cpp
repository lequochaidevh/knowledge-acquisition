#include "socket_server.h"
#include "socket_client.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <atomic>

using namespace HarisLinux;

// =========================================================================
// ASYNCHRONOUS SERVER LOOP SIMULATION (FOR DATAGRAM / STREAM BACKGROUND RUNS)
// =========================================================================
template <typename Transport>
void run_sandbox_server_loop(SocketServer<Transport>& server, std::atomic<bool>& keep_running) {
    PacketHeader         header{};
    std::vector<uint8_t> payload;

    std::cout << "[Sandbox Server] Thread started. Listening for incoming packages...\n";

    while (keep_running) {
        // pull_data automatically handles sending back ACKs if the Feedback mode is enabled
        if (server.pull_data(header, payload)) {
            std::string rx_string(payload.begin(), payload.end());
            std::cout << "[Server RX] Received Seq: " << header.sequence_id
                      << " | Type: " << static_cast<int>(header.type) << " | Content: " << rx_string << "\n";
        }

        // Throttle the loop execution to prevent high CPU utilization
        std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    std::cout << "[Sandbox Server] Thread stopped cleanly.\n";
}

// =========================================================================
// TEST SUITE 1: CONTINUOUS STREAM TRANSMISSION TEST (SOCK_STREAM)
// =========================================================================
TEST(SocketSandboxTest, StreamSequentialTransmission) {
    Ipc::Generic<Ipc::Server> server_flags = Ipc::Server::Feedback;
    Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback;

    std::string socket_path = "/tmp/sandbox_stream.sock";

    // 1. Initialize Stream-based Server and Client components
    SocketServer<IIpc::StreamTag> server(AF_UNIX, SOCK_STREAM, server_flags);
    SocketClient<IIpc::StreamTag> client(AF_UNIX, SOCK_STREAM, client_flags);

    // 2. Provision network bindings
    ASSERT_TRUE(server.start_server(socket_path)) << "Server failed to bind path.";

    // Spawn a worker thread to handle the blocking accept_connection call asynchronously
    std::atomic<bool> accept_done(false);
    std::thread       accept_thread([&]() {
        EXPECT_TRUE(server.accept_connection());
        accept_done = true;
    });

    // Client actively establishes the communication pipeline to the Server
    std::this_thread::sleep_for(std::chrono::milliseconds(10));  // Brief pause to allow server to listen
    ASSERT_TRUE(client.connect_to_server(socket_path)) << "Client failed to connect to server.";

    if (accept_thread.joinable()) {
        accept_thread.join();
    }
    ASSERT_TRUE(accept_done);

    // 3. Execute packet transmission from Client -> Server
    std::string client_msg = "Hello Stream Server!";
    EXPECT_TRUE(client.push_data(DataType::Text, client_msg));

    // Server extracts and validates the payload data
    PacketHeader         server_rx_header{};
    std::vector<uint8_t> server_rx_payload;
    ASSERT_TRUE(server.pull_data(server_rx_header, server_rx_payload));

    std::string server_rx_str(server_rx_payload.begin(), server_rx_payload.end());
    EXPECT_EQ(server_rx_str, client_msg);
    EXPECT_EQ(server_rx_header.sequence_id, 0);  // The initial packet index must start at 0

    // 4. Verify the reverse feedback loop (ACK verification)
    PacketHeader         client_rx_header{};
    std::vector<uint8_t> client_rx_payload;

    // Client invokes pull_data to consume the ACK_SERVER packet sent by the server
    ASSERT_TRUE(client.pull_data(client_rx_header, client_rx_payload));
    EXPECT_EQ(client_rx_header.type, DataType::Heartbeat);
    EXPECT_EQ(client_rx_header.sequence_id, 0);  // Validates the ACK matches packet sequence 0
}

// =========================================================================
// TEST SUITE 2: SEQUENTIAL DATAGRAM TRANSMISSION TEST WITH FEEDBACK (SOCK_DGRAM)
// =========================================================================
TEST(SocketSandboxTest, DatagramSequentialTransmission) {
    Ipc::Generic<Ipc::Server> server_flags = Ipc::Server::Feedback;
    Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback;

    std::string socket_path = "/tmp/UnixSocket.sock";

    // Initialize Server and Client instances explicitly utilizing the DgramTag layout
    SocketServer<IIpc::DgramTag> server(AF_UNIX, SOCK_DGRAM, server_flags);
    SocketClient<IIpc::DgramTag> client(AF_UNIX, SOCK_DGRAM, client_flags);

    ASSERT_TRUE(server.start_server(socket_path)) << "Server DGRAM failed to start.";
    ASSERT_TRUE(server.accept_connection());
    ASSERT_TRUE(client.connect_to_server(socket_path)) << "Client DGRAM failed to connect.";

    // Array of sample messages varying in content and sizing to test alignment stability
    std::vector<std::string> test_messages = {"Hello Dgram Server!", "Packet number two with different size",
                                              "ShortMsg", "Final packet transaction boundary test"};

    PacketHeader         client_rx_header{};
    std::vector<uint8_t> client_rx_payload;

    // Execute sequential packet processing through a continuous transaction loop
    for (size_t i = 0; i < test_messages.size(); ++i) {
        std::string msg = test_messages[i];

        // 1. Client dispatches the payload
        EXPECT_TRUE(client.push_data(DataType::Text, msg)) << "Failed to send packet index: " << i;

        PacketHeader         server_rx_header{};
        std::vector<uint8_t> server_rx_payload;

        // 2. Server extracts data from the base socket buffer allocation
        ASSERT_TRUE(server.pull_data(server_rx_header, server_rx_payload))
            << "Server failed to pull packet index: " << i;

        // 3. Validate network data payload integrity
        std::string rx_str(server_rx_payload.begin(), server_rx_payload.end());
        EXPECT_EQ(rx_str, msg) << "Data mismatch at packet index: " << i;

        // 4. Verify sequence numbers increment predictably without stack memory corruption
        EXPECT_EQ(server_rx_header.sequence_id, static_cast<uint32_t>(i))
            << "Sequence ID mismatch at packet index: " << i;

        std::cout << "[Test Success] Packet [" << i << "] Verified | Seq ID: " << server_rx_header.sequence_id
                  << " | Size: " << server_rx_payload.size() << " bytes\n";
    }
}

// =========================================================================
// TEST SUITE 3: NETWORK TOPOLOGY & ADDRESS CONFIGURATION VALIDATION
// =========================================================================

// Test fixture or direct wrapper classes to expose internal inherited fields
// from your actual SocketClient and SocketServer implementation layouts.
class TestSocketClient : public SocketClient<IIpc::StreamTag> {
 public:
    using SocketClient::_remote_addr;
    using SocketClient::_remote_addr_len;
    using SocketClient::configure_address;
    using SocketClient::SocketClient;
};

class TestSocketServer : public SocketServer<IIpc::StreamTag> {
 public:
    using SocketServer::_remote_addr;
    using SocketServer::_remote_addr_len;
    using SocketServer::configure_address;
    using SocketServer::SocketServer;
};

// Scenario 3.1: Verify local UNIX Domain Socket string formatting maps correctly using Server
TEST(SocketAddressTest, ValidUnixDomainTopologyViaServer) {
    Ipc::Generic<Ipc::Server> server_flags = Ipc::Server::Feedback;
    TestSocketServer          server(AF_UNIX, SOCK_STREAM, server_flags);
    std::string               path = "/tmp/dynamic_test_server.sock";

    ASSERT_TRUE(server.configure_address(path, 0));
    EXPECT_EQ(server._remote_addr_len, sizeof(sockaddr_un));

    auto* un_addr = reinterpret_cast<const sockaddr_un*>(&server._remote_addr);
    EXPECT_EQ(un_addr->sun_family, AF_UNIX);
    EXPECT_STREQ(un_addr->sun_path, path.c_str());
}

// Scenario 3.2: Verify IPv4 parser validation, network byte ordering, and rejections via Client
TEST(SocketAddressTest, IPv4NetworkTopologyViaClient) {
    Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback;
    TestSocketClient          client(AF_INET, SOCK_STREAM, client_flags);
    uint16_t                  test_port = 8080;

    // 1. Validate target structure transformation with an active IPv4 endpoint
    ASSERT_TRUE(client.configure_address("192.168.1.100", test_port));
    EXPECT_EQ(client._remote_addr_len, sizeof(sockaddr_in));

    auto* in_addr = reinterpret_cast<const sockaddr_in*>(&client._remote_addr);
    EXPECT_EQ(in_addr->sin_family, AF_INET);
    EXPECT_EQ(in_addr->sin_port, htons(test_port));

    // Convert address bytes back into a readable string to verify numeric placement
    char ip_str[INET_ADDRSTRLEN];
    ASSERT_NE(inet_ntop(AF_INET, &(in_addr->sin_addr), ip_str, INET_ADDRSTRLEN), nullptr);
    EXPECT_STREQ(ip_str, "192.168.1.100");

    // 2. Verify invalid syntax formats get blocked safely without memory drops
    EXPECT_FALSE(client.configure_address("999.999.999.999", test_port));
    EXPECT_FALSE(client.configure_address("invalid_ip_format", test_port));
}

// Scenario 3.3: Verify IPv6 layout parsing, structure sizing, and shorthand translations via Client
TEST(SocketAddressTest, IPv6NetworkTopologyViaClient) {
    Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback;
    TestSocketClient          client(AF_INET6, SOCK_STREAM, client_flags);
    uint16_t                  test_port = 443;

    // 1. Verify standard loopback execution parsing handles compressed notation (::1)
    ASSERT_TRUE(client.configure_address("::1", test_port));
    EXPECT_EQ(client._remote_addr_len, sizeof(sockaddr_in6));

    auto* in6_addr = reinterpret_cast<const sockaddr_in6*>(&client._remote_addr);
    EXPECT_EQ(in6_addr->sin6_family, AF_INET6);
    EXPECT_EQ(in6_addr->sin6_port, htons(test_port));

    // 2. Verify invalid alphanumeric sequences trigger fallback rejections
    EXPECT_FALSE(client.configure_address("2001::global::garbage::string", test_port));
}

// =========================================================================
// SCENARIO 3.4: VALIDATE ACTUAL PACKET TRANSMISSION OVER IPV4 LOOPBACK
// =========================================================================
TEST(SocketAddressTest, IPv4ActualTransmissionLoopback) {
    Ipc::Generic<Ipc::Server> server_flags = Ipc::Server::Feedback;
    Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback;

    std::string ip_address   = "127.0.0.1";
    int         network_port = 9091;  // Use an unassigned ephemeral port

    // 1. Instantiate true network wrappers using matching StreamTag rules
    SocketServer<IIpc::StreamTag> server(AF_INET, SOCK_STREAM, server_flags);
    SocketClient<IIpc::StreamTag> client(AF_INET, SOCK_STREAM, client_flags);

    // 2. Start network listener context on localhost interface
    ASSERT_TRUE(server.start_server(ip_address, network_port)) << "IPv4 Server failed to bind loopback adapter.";

    // Spawn thread to handle blocking accept operations safely
    std::atomic<bool> accept_complete(false);
    std::thread       accept_thread([&]() {
        EXPECT_TRUE(server.accept_connection());
        accept_complete = true;
    });

    // 3. Client connects to localhost server port via true socket bindings
    std::this_thread::sleep_for(std::chrono::milliseconds(15));  // Delay to verify listening channel is open
    ASSERT_TRUE(client.connect_to_server(ip_address, network_port)) << "IPv4 Client failed to connect to Loopback.";

    if (accept_thread.joinable()) {
        accept_thread.join();
    }
    ASSERT_TRUE(accept_complete);

    // 4. Push network data over loopback interface (This triggers logger outputs)
    std::string network_payload = "IPv4_Loopback_Data_Match";
    EXPECT_TRUE(client.push_data(DataType::Text, network_payload));

    // 5. Server pulls and processes the payload structure
    PacketHeader         server_rx_header{};
    std::vector<uint8_t> server_rx_payload;
    ASSERT_TRUE(server.pull_data(server_rx_header, server_rx_payload));

    // 6. Verify data layout matching integrity across network layers
    std::string received_string(server_rx_payload.begin(), server_rx_payload.end());
    EXPECT_EQ(received_string, network_payload);
    EXPECT_EQ(server_rx_header.sequence_id, 0);

    // 7. Flush feedback queue by pulling the automatic ACK back to the client
    PacketHeader         client_ack_header{};
    std::vector<uint8_t> client_ack_payload;
    ASSERT_TRUE(client.pull_data(client_ack_header, client_ack_payload));
    EXPECT_EQ(client_ack_header.type, DataType::Heartbeat);
}