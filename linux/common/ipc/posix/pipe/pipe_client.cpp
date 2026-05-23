#include "pipe_client.h"

namespace HarisLinux {

void PipeClient::Connect() {
    write_fd = open(pipe_path_main.c_str(), O_WRONLY);
    if (mode == ClientMode::CheckLose) {
        read_fd = open(pipe_path_fb.c_str(), O_RDONLY);
    }
}

void PipeClient::SendData(DataType type, const std::vector<uint8_t>& data) {
    current_seq++;
    SendPacket(write_fd, type, data, current_seq);
}

void PipeClient::CheckFeedback() {
    if (mode != ClientMode::CheckLose) return;
    PacketHeader         fb_header;
    std::vector<uint8_t> fb_data;
    if (ReadPacket(read_fd, fb_header, fb_data)) {
        if (fb_header.sequence_id != current_seq) {
            std::cout << "Data Lost Detected! Expected Seq: " << current_seq << " Got: " << fb_header.sequence_id
                      << std::endl;
        }
    }
}

}  // namespace HarisLinux