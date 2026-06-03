#include "pipe_server.h"

namespace HarisLinux {

static auto logger = LogRegistry::getInstance().getLogger("PipeServer");

PipeServer::PipeServer(const std::string& path, ServerMode m) : PosixPipe(path), _mode(m) {
    logger->setLevel(LogLevel::Trace);
    HARIS_LOG_DEBUG("Listen request at Request Pipe: {} with mkfifo: {}", path, 666);
    unlink(_pipe_path_main.c_str());
    unlink(_pipe_path_fb.c_str());
    mkfifo(_pipe_path_main.c_str(), 0666);
    access(_pipe_path_main.c_str(), F_OK);
    if (_mode == ServerMode::Feedback) {
        mkfifo(_pipe_path_fb.c_str(), 0666);
        access(_pipe_path_fb.c_str(), F_OK);
    }
    HARIS_LOG_DEBUG("Request Pipe avaiable...");
}

PipeServer::~PipeServer() {
    for (auto& [id, fds] : _active_clients) {
        if (fds.first != -1) close(fds.first);
        if (fds.second != -1) close(fds.second);
        std::string dynamic_main_path = "/tmp/pipe_" + id;
        std::string dynamic_fb_path   = dynamic_main_path + "_fb";
        unlink(dynamic_main_path.c_str());
        HARIS_LOG_DEBUG("Removed client pipe: {} ", dynamic_main_path);
        unlink(dynamic_fb_path.c_str());
        HARIS_LOG_DEBUG("Removed client pipe: {} ", dynamic_fb_path);
    }
}

void PipeServer::calculate_hz(uint64_t current_ms) {
    _pkt_count++;
    if (_last_time == 0) _last_time = current_ms;
    if (current_ms - _last_time >= 1000) {
        HARIS_LOG_TRACE("Frequency: {} Hz", _pkt_count);
        _pkt_count = 0;
        _last_time = current_ms;
    }
}

bool PipeServer::accept_client() {
    HARIS_LOG_INFO("Open Request Pipe, wait any client request");

    // Block wait command from client
    int req_read_fd  = open(_pipe_path_main.c_str(), O_RDONLY);
    int req_write_fd = -1;
    if (_mode == ServerMode::Feedback) {
        req_write_fd = open(_pipe_path_fb.c_str(), O_WRONLY);
    }

    PacketHeader         header;
    std::vector<uint8_t> data;

    // Read data from client request
    if (receive_packet(req_read_fd, header, data)) {
        if (header.type == DataType::Command && data.size() == sizeof(IPCRequestPayload)) {
            IPCRequestPayload req;
            std::memcpy(&req, data.data(), sizeof(IPCRequestPayload));

            std::string client_id(req.client_id, strnlen(req.client_id, sizeof(req.client_id)));
            HARIS_LOG_INFO("Get client request: {} ", client_id);

            {
                // Check ID has existed:
                std::lock_guard<std::mutex> lock(_active_clients_mutex);

                if (_active_clients.find(client_id) != _active_clients.end()) {
                    HARIS_LOG_DEBUG("Founded Client {} in the container", client_id);
                    // ================= SHIFT LOGIC: PROCESS COMMAND REMOVE =================
                    if (req.command == 2) {
                        HARIS_LOG_DEBUG("Get REMOVE (command = 2) request from client: {} ", client_id);

                        if (_mode == ServerMode::Feedback && req_write_fd != -1) {
                            std::string_view reject_msg = "REMOVED";
                            send_packet(req_write_fd, DataType::Command, reject_msg, header.sequence_id);
                        }

                        // 1. Close all File Descriptors for this remove request for this Client
                        int dyn_read_fd  = _active_clients[client_id].first;
                        int dyn_write_fd = _active_clients[client_id].second;

                        if (dyn_read_fd != -1) close(dyn_read_fd);
                        if (dyn_write_fd != -1) close(dyn_write_fd);

                        // 2. Remove all FIFO file in the OS.
                        std::string dynamic_main_path = "/tmp/pipe_" + client_id;
                        std::string dynamic_fb_path   = dynamic_main_path + "_fb";
                        unlink(dynamic_main_path.c_str());
                        unlink(dynamic_fb_path.c_str());

                        // 3. Remove element of the map.
                        _active_clients.erase(client_id);
                        HARIS_LOG_DEBUG("Removed successfully Client");

                        // Clean up Request Pipe of this command.
                        close(req_read_fd);
                        if (req_write_fd != -1) close(req_write_fd);
                        return false;  // todo: remove delay
                    }
                    // =================================================================

                    HARIS_LOG_ERROR("Client ID have been dupplicated request !");
                    HARIS_LOG_ERROR("REJECT new Client: {} ", client_id);

                    // If The request dupplicated a id (REJECTED) for Client
                    if (_mode == ServerMode::Feedback && req_write_fd != -1) {
                        std::string_view reject_msg = "REJECTED";
                        send_packet(req_write_fd, DataType::Command, reject_msg, header.sequence_id);
                    }

                    close(req_read_fd);
                    if (req_write_fd != -1) close(req_write_fd);
                    return false;
                }
            }
            // 1. Get ID and Create pipe for that client.
            std::string dynamic_main_path = "/tmp/pipe_" + client_id;
            std::string dynamic_fb_path   = dynamic_main_path + "_fb";

            unlink(dynamic_main_path.c_str());
            unlink(dynamic_fb_path.c_str());
            mkfifo(dynamic_main_path.c_str(), 0666);
            mkfifo(dynamic_fb_path.c_str(), 0666);

            // 2. Send ACK to notify for the client "the pipe have been created successfully"
            if (_mode == ServerMode::Feedback && req_write_fd != -1) {
                std::string_view ack_msg = "OPENED";
                send_packet(req_write_fd, DataType::Command, ack_msg, header.sequence_id);
            }

            // 3. Close Request Pipe (and Refresh that).
            close(req_read_fd);
            if (req_write_fd != -1) close(req_write_fd);

            // 4. Open new pipe (server - client_id)
            HARIS_LOG_DEBUG("Wait client join the /tmp/pipe_{} pipe ...", client_id);
            int dyn_read_fd = open(dynamic_main_path.c_str(), O_RDONLY | O_NONBLOCK);
            // block
            int dyn_write_fd = open(dynamic_fb_path.c_str(), O_WRONLY);
            {
                std::lock_guard<std::mutex> lock(_active_clients_mutex);
                // Store the pair File Descriptors for that Client ID to monitor data.
                _active_clients[client_id] = {dyn_read_fd, dyn_write_fd};
            }
            HARIS_LOG_DEBUG("Connected to client successfully: {} ", client_id);
            return true;
        }
    }

    // Close fd and clean up if appear err
    close(req_read_fd);
    if (req_write_fd != -1) close(req_write_fd);
    return false;
}

void PipeServer::process_client_packet(const std::string& client_id, int read_fd, int write_fd) {
    PacketHeader         header;
    std::vector<uint8_t> data;

    // 1. Get pack
    if (receive_packet(read_fd, header, data)) {
        calculate_hz(header.timestamp_ms);

        HARIS_LOG_INFO("[{}] === New Packet Received === ", client_id);
        HARIS_LOG_DEBUG("Type: {} - Size: {} bytes - Seq {} ",       // format data debug
                        static_cast<int>(header.type),               // type
                        static_cast<uint32_t>(header.payload_size),  // size of payload
                        static_cast<uint32_t>(header.sequence_id));  // id

        // 2. Check
        if (data.empty()) {
            HARIS_LOG_DEBUG("Data: [Empty payload]");
        } else {
            HARIS_LOG_DEBUG("Data: ");

            // 3. Implement with data type
            switch (header.type) {
                case DataType::Text: {
                    std::string text_msg(data.begin(), data.end());
                    HARIS_LOG_DEBUG("\"{}\"", text_msg);
                    break;
                }
                case DataType::Number: {
                    if (data.size() >= sizeof(int)) {
                        int number = 0;
                        std::memcpy(&number, data.data(), sizeof(int));
                        HARIS_LOG_DEBUG("\"{}\"", number);
                    } else {
                        HARIS_LOG_ERROR("Invalid Number Size");
                    }
                    break;
                }
                case DataType::Media:
                case DataType::Command: {
                    HARIS_LOG_DEBUG("[Hex Display]:");
                    for (size_t i = 0; i < std::min(data.size(), size_t(16)); ++i) {
                        printf("%02X ", data[i]);
                    }
                    if (data.size() > 16)
                        HARIS_LOG_DEBUG(" ...");
                    else
                        printf("\n");
                    break;
                }
                default:
                    HARIS_LOG_WARN("Unknown Data Type");
                    break;
            }
        }

        // 4. Feedback ACK
        if (_mode == ServerMode::Feedback) {
            std::string_view empty_ack = "";
            send_packet(write_fd, DataType::Command, empty_ack, header.sequence_id);
        }
    }
}

void PipeServer::poll_all_clients() {
    // Create thread because it will block at open pipe
    std::thread acceptor_thread([this]() {
        while (true) {
            // auto block to wait new client, and add it to <_active_clients> container
            if (!accept_client())
                // wait OS clean up
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    acceptor_thread.detach();  // Trigger thread

    // polling all client connected but non-block read
    while (true) {
        // Loop all element in map: _active_clients
        // Key: client_id, Value: std::pair<int, int> contain {read_fd, write_fd}
        {
            std::lock_guard<std::mutex> lock(_active_clients_mutex);
            for (const auto& [client_id, fds] : _active_clients) {
                int read_fd  = fds.first;
                int write_fd = fds.second;

                // check fd existing
                if (read_fd != -1) {
                    process_client_packet(client_id, read_fd, write_fd);
                }
            }
        }
        // Save CPU resource
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }
}

}  // namespace HarisLinux
