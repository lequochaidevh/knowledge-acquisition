#include "pipe_server.h"

namespace HarisLinux {

void PipeServer::start() {
    if (_mode == ServerMode::ReadOnly || _mode == ServerMode::Feedback) {
        _read_fd = open(_pipe_path_main.c_str(), O_RDONLY);
    }
    if (_mode == ServerMode::Feedback) {
        _write_fd = open(_pipe_path_fb.c_str(), O_WRONLY);
    }
    // note: Broadcast with Pipe Named need multi pipe arch,
    // at the present, only process main thread
}

void PipeServer::process_data_incoming() {
    PacketHeader         header;
    std::vector<uint8_t> data;

    // 1. Receive from PosixPipe
    if (receive_packet(_read_fd, header, data)) {
        calculate_hz(header.timestamp_ms);

        // Show header package infomation.
        std::cout << "\n[Server] === New Packet Received ===" << std::endl;
        std::cout << "  Type: " << static_cast<int>(header.type) << " | Size: " << header.payload_size << " bytes"
                  << " | Seq: " << header.sequence_id << std::endl;

        // 2. check payload
        if (data.empty()) {
            std::cout << "  Data: [Empty payload]" << std::endl;
        } else {
            std::cout << "  Data: ";

            // 3. Reset data with byte array
            switch (header.type) {
                case DataType::Text: {
                    // convert byte to std::string
                    std::string text_msg(data.begin(), data.end());
                    std::cout << "\"" << text_msg << "\"" << std::endl;
                    break;
                }
                case DataType::Number: {
                    // cast byte array to interger safety with memcpy
                    if (data.size() >= sizeof(int)) {
                        int number = 0;
                        std::memcpy(&number, data.data(), sizeof(int));
                        std::cout << number << std::endl;
                    } else {
                        std::cout << "[Invalid Number Size]" << std::endl;
                    }
                    break;
                }
                case DataType::Media:
                case DataType::Command: {
                    // show Hex of the raw data (Image, Video, System command)
                    std::cout << "[Hex Display]: ";
                    for (size_t i = 0; i < std::min(data.size(), size_t(16)); ++i) {
                        printf("%02X ", data[i]);
                    }
                    if (data.size() > 16) std::cout << "...";
                    std::cout << std::endl;
                    break;
                }
                default:
                    std::cout << "[Unknown Data Type]" << std::endl;
                    break;
            }
        }

        // 4. Feedback ACK to Client by new function (Send with template format).
        if (_mode == ServerMode::Feedback) {
            // Use string_view to make payload ACK lightly for memory.
            std::string_view empty_ack = "";
            send_packet(_write_fd, DataType::Command, empty_ack, header.sequence_id);
        }
    }
}

}  // namespace HarisLinux