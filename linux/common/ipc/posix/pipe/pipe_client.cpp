#include "pipe_client.h"

namespace HarisLinux {

void PipeClient::connect() {
    _write_fd = open(_pipe_path_main.c_str(), O_WRONLY);
    if (_mode == ClientMode::CheckLose) {
        _read_fd = open(_pipe_path_fb.c_str(), O_RDONLY);
    }
}

void PipeClient::check_feedback() {
    if (_mode != ClientMode::CheckLose) return;
    PacketHeader         fb_header;
    std::vector<uint8_t> fb_data;
    if (receive_packet(_read_fd, fb_header, fb_data)) {
        if (fb_header.sequence_id != _current_seq) {
            std::cout << "Data Lost Detected! Expected Seq: " << _current_seq << " Got: " << fb_header.sequence_id
                      << std::endl;
        }
    }
}

}  // namespace HarisLinux