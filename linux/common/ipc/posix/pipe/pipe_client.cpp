#include "pipe_client.h"

namespace HarisLinux {

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

bool PipeClient::request_and_switch_pipe() {
    std::cout << "[Client][" << _client_id << "] have connected to Request Pipe ...\n";
    _write_fd = open(_pipe_path_main.c_str(), O_WRONLY);
    if (_mode == ClientMode::CheckLose) {
        _read_fd = open(_pipe_path_fb.c_str(), O_RDONLY);
    }

    // Prepare Payload
    PipeRequestPayload req;
    std::strncpy(req.client_id, _client_id.c_str(), sizeof(req.client_id));
    req.command = 1;  // Command = 1 to request open new pipe

    std::cout << "[Client][" << _client_id << "] Send request to pipe ...\n";
    _current_seq++;
    std::string_view req_view(reinterpret_cast<const char*>(&req), sizeof(PipeRequestPayload));
    send_packet(_write_fd, DataType::Command, req_view, _current_seq);

    // Wait Server to accept and feedback ACK.
    if (_mode == ClientMode::CheckLose) {
        PacketHeader         fb_header;
        std::vector<uint8_t> fb_payload;
        if (receive_packet(_read_fd, fb_header, fb_payload)) {
            std::string_view ack_status(reinterpret_cast<const char*>(fb_payload.data()), fb_payload.size());
            if (ack_status == "REJECTED") {
                std::cout << "[Client][" << _client_id
                          << "] ERROR: The ID have existed in Server ! Reject to connection." << std::endl;
                close(_write_fd);
                if (_read_fd != -1) close(_read_fd);
                throw "Request failed";
                // return false;
            }
            // If server accepted request
            if (fb_header.sequence_id == _current_seq) {
                std::cout << "[Client][" << _client_id << "] Server have opened new pipe. Switch to it...\n";

                // Close request_pipe fd
                close(_write_fd);
                if (_read_fd != -1) close(_read_fd);

                // Update to new pipe
                _pipe_path_main = "/tmp/pipe_" + _client_id;
                _pipe_path_fb   = _pipe_path_main + "_fb";

                // Connect to new pipe
                _write_fd = open(_pipe_path_main.c_str(), O_WRONLY | O_NONBLOCK);
                _read_fd  = open(_pipe_path_fb.c_str(), O_RDONLY);
                std::cout << "[Client][" << _client_id << "] Switch to new pipe successfully!\n";
                return true;
            }
        }
    }
    close(_write_fd);
    if (_read_fd != -1) close(_read_fd);
    return false;
}

}  // namespace HarisLinux