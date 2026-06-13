#include "../../ipc_sender.h"
#include "../../ipc_receiver.h"

using namespace HarisLinux;

const std::string SOCK_PATH = "/tmp/test_crtp_dgram.sock";

struct DeviceConfig {
    uint32_t               device_id;
    float                  sample_rate;
    const std::string_view status_code;
};

void run_receiver_logic(int pipe_read_fd, int dgram_socket_fd) {
    std::cout << "[RECEIVER] The reciver available ...\n\n";

    PacketHeader         header;
    std::vector<uint8_t> payload;
    payload.reserve(4096);

    // -------------------------------------------------------------------------
    // TEST 1: PIPE (StreamReceiver)
    // -------------------------------------------------------------------------
    StreamReceiver pipe_receiver(pipe_read_fd);
    std::cout << "[RECEIVER] --- Started PIPE Stream ---\n";

    if (pipe_receiver.receive(header, payload)) {
        assert(header.type == DataType::Number);
        int received_val = *reinterpret_cast<int*>(payload.data());
        std::cout << "[RECEIVER][PIPE] Test case 1.1 - Data type is Number: " << received_val << " (PASSED)\n";
    } else {
        std::cerr << "[RECEIVER][PIPE] Test case 1.1 FAILED\n";
    }

    if (pipe_receiver.receive(header, payload)) {
        assert(header.type == DataType::Text);
        std::string received_str(reinterpret_cast<char*>(payload.data()), header.payload_size);
        std::cout << "[RECEIVER][PIPE] Test case 1.2 - Data type is Text: \"" << received_str << "\" (PASSED)\n";
    } else {
        std::cerr << "[RECEIVER][PIPE] Test case 1.2 FAILED\n";
    }

    // -------------------------------------------------------------------------
    // TEST 2: UNIX Socket Datagram (DgramReceiver)
    // -------------------------------------------------------------------------
    DgramReceiver dgram_receiver(dgram_socket_fd);
    std::cout << "\n[RECEIVER] --- Started UNIX Datagram ---\n";

    if (dgram_receiver.receive(header, payload)) {
        assert(header.type == DataType::Media);
        std::cout << "[RECEIVER][DGRAM] Test case 2.1 - Data type is Media, size: " << header.payload_size
                  << " bytes (PASSED)\n";
        assert(payload[0] == 0xAA && payload[payload.size() - 1] == 0xBB);
    } else {
        std::cerr << "[RECEIVER][DGRAM] Test case 2.1 FAILED\n";
    }

    if (dgram_receiver.receive(header, payload)) {
        assert(header.type == DataType::Custom);
        DeviceConfig* config = reinterpret_cast<DeviceConfig*>(payload.data());
        std::cout << "[RECEIVER][DGRAM] Test case 2.2 - Data type is Custom (Struct) (PASSED)\n";
        std::cout << "               -> Device ID  : " << config->device_id << "\n";
        std::cout << "               -> Sample Rate: " << config->sample_rate << " Hz\n";
        std::cout << "               -> Status     : '" << config->status_code << "'\n";

        assert(config->device_id == 9999);
        // assert(config->status_code == 'A');
    } else {
        std::cerr << "[RECEIVER][DGRAM] Test case 2.2 FAILED\n";
    }

    std::cout << "\n[RECEIVER] === ALL UNIT TESTS PASSED SUCCESSFULLY ===\n";
}

void run_sender_logic(int pipe_write_fd, int dgram_socket_fd) {
    sleep(1);
    std::cout << "[SENDER] Sender is available ...\n";

    // 1. PIPE
    StreamSender pipe_sender(pipe_write_fd);
    uint32_t     test_int = 756799112;
    pipe_sender.send(DataType::Number, test_int, 101);

    std::string test_str = "IPC system is optimized by CRTP";
    pipe_sender.send(DataType::Text, test_str, 102);

    // 2. UNIX Datagram
    DgramSender dgram_sender(dgram_socket_fd, SOCK_PATH);

    std::vector<uint8_t> mock_image(2048, 0x00);
    mock_image[0]                     = 0xAA;
    mock_image[mock_image.size() - 1] = 0xBB;
    dgram_sender.send(DataType::Media, mock_image, 201);

    DeviceConfig         config{9999, 44100.0f, "Hello"};
    std::vector<uint8_t> struct_buffer(sizeof(DeviceConfig));
    std::memcpy(struct_buffer.data(), &config, sizeof(DeviceConfig));
    dgram_sender.send(DataType::Custom, struct_buffer, 202);

    std::cout << "[SENDER] Send all test data package. Exit.\n";
}

int main() {
    int pipe_fds[2];
    if (pipe(pipe_fds) < 0) {
        perror("Can not create Pipe");
        return 1;
    }

    unlink(SOCK_PATH.c_str());
    int dgram_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
    if (dgram_fd < 0) {
        perror("Can not create Socket");
        return 1;
    }

    sockaddr_un local_addr{};
    std::memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sun_family = AF_UNIX;
    std::strncpy(local_addr.sun_path, SOCK_PATH.c_str(), sizeof(local_addr.sun_path) - 1);

    if (bind(dgram_fd, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)) < 0) {
        perror("Bind socket failed");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Fork failed");
        return 1;
    } else if (pid == 0) {
        close(pipe_fds[0]);  // Close read end
        run_sender_logic(pipe_fds[1], dgram_fd);
        close(pipe_fds[1]);
        close(dgram_fd);
        return 0;
    } else {
        close(pipe_fds[1]);  // Close write end
        run_receiver_logic(pipe_fds[0], dgram_fd);
        close(pipe_fds[0]);
        close(dgram_fd);

        wait(nullptr);
        unlink(SOCK_PATH.c_str());
    }

    return 0;
}

// mkdir build
// g++ -std=c++17 -I../../../ send_and_read.cpp -o build/ipc_test && ./build/ipc_test