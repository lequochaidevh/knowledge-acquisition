#include "pipe_client.h"

namespace HarisLinux {

// static auto logger = LogRegistry::getInstance().getLogger("PipeClient");

PipeClient::PipeClient(const std::string& request_path, const std::string& id, Ipc::Generic<Ipc::Client> modes)
    :  //
      PosixPipe<Ipc::Client>(request_path, modes),
      _old_upstream_path(request_path),
      _client_id(id) {
    INIT_LOGGER("PipeClient");
    logger->setLevel(LogLevel::Trace);
}

PipeClient::~PipeClient() {
    HARIS_LOG_DEBUG(
        "The client id [{}]  Distructor class, "
        "send command REMOVE to Server...",
        _client_id);
    request_and_switch_main_pipe();
}

void PipeClient::check_feedback() {
    if (_modes.missing(Ipc::Client::CheckLose)) return;
    PacketHeader         fb_header;
    std::vector<uint8_t> fb_data;
    if (receive_packet(_read_share_fd, fb_header, fb_data)) {
        if (fb_header.sequence_id != _current_seq) {
            HARIS_LOG_DEBUG(
                "Data Lost Detected! Expected Seq: {} "
                "Got: {}",
                _current_seq, static_cast<uint32_t>(fb_header.sequence_id));
        } else {
            _dispatcher.dispatch(fb_header, fb_data);
        }
    }
}

bool PipeClient::request_and_switch_main_pipe() {
    _upstream_path   = _old_upstream_path;
    _downstream_path = _old_upstream_path + "_fb";

    // _write_share_fd.reset();
    // if (_read_share_fd != -1) close(_read_share_fd);

    HARIS_LOG_DEBUG(
        "Request with old main pipe: "
        "Send: {}",
        "Read: {}",  //
        _upstream_path, _downstream_path);

    request_and_switch_pipe(2);
    return true;
}

bool PipeClient::request_and_switch_pipe(uint32_t command_arg) {
    HARIS_LOG_DEBUG(
        "The client id: [{}] "
        "have connected to Request Pipe ...",
        _client_id);
    _write_share_fd = SharedFileDescription<PipePolicy>(open(_upstream_path.c_str(), O_WRONLY));

    if (_modes & Ipc::Client::CheckLose) {
        _read_share_fd = SharedFileDescription<PipePolicy>(open(_downstream_path.c_str(), O_RDONLY));
    }

    // Prepare Payload
    IPCRequestPayload req;
    std::strncpy(req.client_id, _client_id.c_str(), sizeof(req.client_id));
    req.command = command_arg;  // Command = 1 to request open new pipe

    HARIS_LOG_DEBUG(
        "The client id: [{}] "
        "Send request to pipe ...",
        _client_id);
    _current_seq++;

    if (!send_packet(_write_share_fd, DataType::Command, req, _current_seq))  // Request to main pipe
        HARIS_LOG_CRITICAL("Send packet failed");

    // Wait Server to accept and feedback ACK.
    if (_modes & Ipc::Client::CheckLose) {
        PacketHeader         fb_header;
        std::vector<uint8_t> fb_payload;
        if (receive_packet(_read_share_fd, fb_header, fb_payload)) {
            std::string_view ack_status(reinterpret_cast<const char*>(fb_payload.data()), fb_payload.size());
            if (ack_status == "REJECTED") {  // when command = 1
                HARIS_LOG_ERROR(
                    "Command:[1] --- The client id: [{}] "
                    "have existed in Server ! Reject to connection ...",
                    _client_id);
                // _write_share_fd.reset();
                // if (_read_share_fd) close(_read_share_fd);
                throw "Request failed";
                // return false;
            } else if (ack_status == "REMOVED") {
                HARIS_LOG_INFO("Removed pipe successfully");
                return true;
            }
            // If server accepted request
            if (fb_header.sequence_id == _current_seq) {
                HARIS_LOG_DEBUG(
                    "The client id: [{}] "
                    "Server have opened new pipe. Switch to it...",
                    _client_id);
                // Close request_pipe fd
                // _write_share_fd.reset();
                // if (_read_share_fd != -1) close(_read_share_fd);

                // Update to new pipe
                _upstream_path   = "/tmp/pipe_" + _client_id;
                _downstream_path = _upstream_path + "_fb";

                // Connect to new pipe
                _write_share_fd = SharedFileDescription<PipePolicy>(open(_upstream_path.c_str(), O_WRONLY));
                _read_share_fd  = SharedFileDescription<PipePolicy>(open(_downstream_path.c_str(), O_RDONLY));
                HARIS_LOG_DEBUG(
                    "The client id: [{}] "
                    "Switch to new pipe successfully!",
                    _client_id);
                return true;
            }
        }
    }
    // _write_share_fd.reset();
    // if (_read_share_fd != -1) close(_read_share_fd);
    return false;
}

}  // namespace HarisLinux
