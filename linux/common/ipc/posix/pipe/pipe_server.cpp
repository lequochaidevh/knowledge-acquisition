#include "pipe_server.h"

namespace HarisLinux {

void PipeServer::Start() {
    if (mode == ServerMode::ReadOnly || mode == ServerMode::Feedback) {
        read_fd = open(pipe_path_main.c_str(), O_RDONLY);
    }
    if (mode == ServerMode::Feedback) {
        write_fd = open(pipe_path_fb.c_str(), O_WRONLY);
    }
    // note: Broadcast with Pipe Named need multi pipe arch,
    // at the present, only process main thread
}

void PipeServer::ProcessIncoming() {
    PacketHeader         header;
    std::vector<uint8_t> data;

    // Read data from pipe
    if (ReadPacket(read_fd, header, data)) {
        // 1. Info
        std::cout << "\n[Server] === New Packet Received ===" << std::endl;
        std::cout << "  Type: " << static_cast<int>(header.type) << " | Size: " << header.payload_size << " bytes"
                  << " | Seq: " << header.sequence_id << std::endl;

        // 2. Check
        if (data.empty()) {
            std::cout << "  Data: [Empty payload]" << std::endl;
        } else {
            std::cout << "  Data: ";

            // 3. Check
            switch (header.type) {
                case DataType::Text: {
                    // Convert byte array to string and cout.
                    std::string text_msg(data.begin(), data.end());
                    std::cout << "\"" << text_msg << "\"" << std::endl;
                    break;
                }
                case DataType::Number: {
                    // Case byte to int
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
                    // Media (Image/Video), printf Hex
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

        // Send sequence_id again to client check
        if (mode == ServerMode::Feedback) {
            std::vector<uint8_t> fb_data;
            SendPacket(write_fd, DataType::Command, fb_data, header.sequence_id);
        }
    }
}
}  // namespace HarisLinux