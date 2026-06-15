#include "unix_socket.h"
// Include other matching headers like packet_header.h if required by your layout

using namespace HarisLinux;

int main() {
    std::cout << "==================================================\n";
    std::cout << "STARTING BASIC INTEGRATION TEST WITH HARIS_LINUX LIB\n";
    std::cout << "==================================================\n";

    // ---------------------------------------------------------------------
    // TEST 1: COMPILE-TIME METRICS & INHERITANCE TREE VALIDATION
    // ---------------------------------------------------------------------
    // Verifying that our type-trait conditional inheritance maps safely
    static_assert(std::is_base_of_v<HarisLinux::StreamSender, HarisLinux::UnixSocket<Ipc::Server>>,
                  "Error: Server instance must inherit from StreamSender base class!");

    static_assert(!std::is_base_of_v<HarisLinux::DgramSender, HarisLinux::UnixSocket<Ipc::Server>>,
                  "Error: Server instance should NOT allocate memory for DgramSender!");

    std::cout << "[SUCCESS] Compile-time layout constraints validated smoothly.\n";

    // Setup clear, distinct compile-time flags matching respective template structures
    Ipc::Generic<Ipc::Server> server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;
    Ipc::Generic<Ipc::Client> client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;

    uint32_t sample_seq = 777;

    // =========================================================================
    // SCENARIO 1: COMPLETE STREAM (TCP/PIPE) SEND & RECEIVE LOOP
    // =========================================================================
    std::cout << "\n[STAGE 1] Testing Stream (SOCK_STREAM) Infrastructure...\n";

    HarisLinux::UnixSocket<Ipc::Server, Ipc::StreamTag> stream_socket(AF_UNIX, SOCK_STREAM, server_flags);

    int pipe_fds[2];
    if (::pipe(pipe_fds) == -1) {
        std::cerr << "Failed to allocate OS pipe markers\n";
        return -1;
    }
    int stream_write_fd = pipe_fds[1];
    int stream_read_fd  = pipe_fds[0];

    std::string stream_tx_data = "HarisStreamDataPayload";

    bool stream_send_ok =
        stream_socket.send_packet(stream_write_fd, HarisLinux::DataType::Text, stream_tx_data, sample_seq);
    assert(stream_send_ok == true);

    HarisLinux::PacketHeader stream_header{};
    std::vector<uint8_t>     stream_rx_payload;

    bool stream_recv_ok = stream_socket.receive_packet(stream_read_fd, stream_header, stream_rx_payload);
    assert(stream_recv_ok == true);

    std::string stream_rx_data(stream_rx_payload.begin(), stream_rx_payload.end());

    // FIX: Using sequence_id to match your actual PacketHeader property
    assert(stream_header.sequence_id == sample_seq);
    assert(stream_rx_data == stream_tx_data);

    ::close(stream_write_fd);
    ::close(stream_read_fd);
    std::cout << "-> [SUCCESS] Stream verification loop finished with total integrity.\n";

    // =========================================================================
    // SCENARIO 2: COMPLETE DATAGRAM (UDP/SOCKET) SEND & RECEIVE LOOP
    // =========================================================================
    std::cout << "\n[STAGE 2] Testing Datagram (SOCK_DGRAM) Infrastructure...\n";

    // 1. Prepare clean, physical system endpoint paths for the UNIX sockets
    const std::string server_path = "/tmp/UnixSocket.sock";
    const std::string client_path = "/tmp/UnixSocket.sock";

    // Ensure old tracking remnants are removed from the filesystem boundary
    ::unlink(server_path.c_str());
    ::unlink(client_path.c_str());

    // 2. Provision two standard, raw UNIX datagram socket descriptors
    int dgram_server_fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
    int dgram_client_fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
    if (dgram_server_fd == -1 || dgram_client_fd == -1) {
        std::cerr << "Failed to provision system socket contexts\n";
        return -1;
    }

    // 3. Bind the server socket to its physical filesystem path descriptor
    sockaddr_un server_addr{};
    server_addr.sun_family = AF_UNIX;
    std::strncpy(server_addr.sun_path, server_path.c_str(), sizeof(server_addr.sun_path) - 1);
    if (::bind(dgram_server_fd, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        std::cerr << "Failed to execute server channel bind sequencing\n";
        return -1;
    }

    // 4. Connect the client socket context explicitly to the target server address loop
    if (::connect(dgram_client_fd, reinterpret_cast<const sockaddr*>(&server_addr), sizeof(server_addr)) == -1) {
        std::cerr << "Failed to connect client pipe to target server destination\n";
        return -1;
    }

    // 5. Initialize your class framework instance (It initializes DgramSender to look at server_path)
    HarisLinux::UnixSocket<Ipc::Client, Ipc::DgramTag> dgram_socket(AF_UNIX, SOCK_DGRAM, client_flags);

    std::string dgram_tx_data = "HarisDgramDataPayload";

    // 6. Transmit through the connected client descriptor.
    // This will now pass 100% cleanly through your base sendto loop!
    bool dgram_send_ok =
        dgram_socket.send_packet(dgram_client_fd, HarisLinux::DataType::Text, dgram_tx_data, sample_seq + 1);
    assert(dgram_send_ok == true);

    HarisLinux::PacketHeader dgram_header{};
    std::vector<uint8_t>     dgram_rx_payload;

    // 7. Extract the packet from the server's descriptor where the frame was routed
    bool dgram_recv_ok = dgram_socket.receive_packet(dgram_server_fd, dgram_header, dgram_rx_payload);
    assert(dgram_recv_ok == true);

    std::string dgram_rx_data(dgram_rx_payload.begin(), dgram_rx_payload.end());
    assert(dgram_header.sequence_id == sample_seq + 1);
    assert(dgram_rx_data == dgram_tx_data);

    // 8. Secure cleanup pipelines to prevent unlinked resource leaks
    ::close(dgram_client_fd);
    ::close(dgram_server_fd);
    ::unlink(server_path.c_str());

    std::cout << "-> [SUCCESS] Datagram verification loop finished with total integrity.\n";
}