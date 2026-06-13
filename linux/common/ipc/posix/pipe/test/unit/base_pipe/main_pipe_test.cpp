#include "posix_pipe.h"

using namespace HarisLinux;

// test Custom Data Struct
struct SensorPayload {
    uint64_t timestamp;
    double   temperature;
    uint32_t error_code;
};

const std::string MAIN_PIPE_PATH = "/tmp/haris_pipe_main";

int main() {
    std::cout << "[INIT] Init FIFO (Named Pipes)...\n";

    unlink(MAIN_PIPE_PATH.c_str());

    if (mkfifo(MAIN_PIPE_PATH.c_str(), 0666) < 0) {
        perror("Error when create mkfifo");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork Failed");
        return 1;
    } else if (pid == 0) {
        // Child process
        std::cout << "[SENDER] Child Process open Pipe to write...\n";

        // Mở đầu ghi (Lệnh open này sẽ block cho đến khi đầu đọc phía Cha được mở)
        int write_fd = open(MAIN_PIPE_PATH.c_str(), O_WRONLY);
        if (write_fd < 0) {
            perror("[SENDER] Can not open write_fd");
            return 1;
        }

        HarisLinux::PosixPipe pipe_sender(MAIN_PIPE_PATH);
        pipe_sender.set_write_fd(write_fd);

        // Test case 1:
        int system_code = 2026;
        std::cout << "[SENDER] -> Sending Number: " << system_code << "\n";
        pipe_sender.send(DataType::Number, system_code, 1);

        // Test case 2:
        std::string alert_msg = "HarisLinux Pipe CRTP Engine Running!";
        std::cout << "[SENDER] -> Sending Text: \"" << alert_msg << "\"\n";
        pipe_sender.send(DataType::Text, alert_msg, 2);

        // Test case 3:
        SensorPayload        sensor_data{123456789, 36.5, 0};
        std::vector<uint8_t> struct_buffer(sizeof(SensorPayload));
        std::memcpy(struct_buffer.data(), &sensor_data, sizeof(SensorPayload));

        std::cout << "[SENDER] -> Sending Custom Struct ...\n";
        pipe_sender.send_packet(pipe_sender.get_write_fd(), DataType::Custom, struct_buffer, 3);

        std::cout << "[SENDER] Send all packages. Process done\n";
        close(write_fd);
        return 0;
    } else {
        // Parent process
        std::cout << "[RECEIVER] Parent process open Pipe to read...\n";

        int read_fd = open(MAIN_PIPE_PATH.c_str(), O_RDONLY);
        if (read_fd < 0) {
            perror("[RECEIVER] Can not open read_fd");
            return 1;
        }

        HarisLinux::PosixPipe pipe_receiver(MAIN_PIPE_PATH);
        pipe_receiver.set_read_fd(read_fd);

        PacketHeader         header;
        std::vector<uint8_t> payload;

        std::cout << "\n[RECEIVER] --- Started read process ---\n";

        // Test case 1
        if (pipe_receiver.receive(header, payload)) {
            assert(header.type == DataType::Number);
            int val = *reinterpret_cast<int*>(payload.data());
            std::cout << "[RECEIVER][PASSED] Get successfully with Number: " << val << "\n";
            assert(val == 2026);
        } else {
            std::cerr << "[RECEIVER][FAILED] Error Test case 1\n";
        }

        // Test case 2
        if (pipe_receiver.receive_packet(pipe_receiver.get_read_fd(), header, payload)) {
            assert(header.type == DataType::Text);
            std::string text(reinterpret_cast<char*>(payload.data()), header.payload_size);
            std::cout << "[RECEIVER][PASSED] Get successfully with Text: \"" << text << "\"\n";
            assert(text == "HarisLinux Pipe CRTP Engine Running!");
        } else {
            std::cerr << "[RECEIVER][FAILED] Error Test case 2\n";
        }

        // Test case 3
        if (pipe_receiver.receive_packet(pipe_receiver.get_read_fd(), header, payload)) {
            assert(header.type == DataType::Custom);
            SensorPayload* received_struct = reinterpret_cast<SensorPayload*>(payload.data());
            std::cout << "[RECEIVER][PASSED] Get successfully with Custom Struct:\n";
            std::cout << "                 -> Temp: " << received_struct->temperature << " C\n";
            std::cout << "                 -> Code: " << received_struct->error_code << "\n";

            assert(received_struct->temperature == 36.5);
            assert(received_struct->error_code == 0);
        } else {
            std::cerr << "[RECEIVER][FAILED] Error Test case 3\n";
        }

        std::cout << "\n[RECEIVER] === All Test case done - PIPE PASSED 100% ===\n";

        close(read_fd);

        // wait child process end
        wait(nullptr);
        unlink(MAIN_PIPE_PATH.c_str());  // Removed file pipe in store
    }

    return 0;
}