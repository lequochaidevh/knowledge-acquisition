#include "pipe_server.h"
namespace HarisLinux {

PipeServer::PipeServer(const std::string& path, Ipc::Generic<Ipc::Server> modes)
    :  //
      PosixPipe<Ipc::Server>(path, modes) {
    INIT_LOGGER("PipeServer");
    logger->setLevel(LogLevel::Trace);
    HARIS_LOG_INFO("Listen request at Request Pipe: {} with mkfifo: {}", path, 666);
    unlink(_upstream_path.c_str());
    unlink(_downstream_path.c_str());
    mkfifo(_upstream_path.c_str(), 0666);
    access(_upstream_path.c_str(), F_OK);
    if (_modes & Ipc::Server::Feedback) {
        mkfifo(_downstream_path.c_str(), 0666);
        access(_downstream_path.c_str(), F_OK);
    }
    HARIS_LOG_INFO("Request Pipe available...");
}

PipeServer::~PipeServer() {
    for (auto& [id, fds] : _client_registry) {
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

void PipeServer::monitor_throughput(uint64_t current_ms) {
    _accumulated_packet_count++;
    if (_last_metrics_timestamp_ms == 0) _last_metrics_timestamp_ms = current_ms;
    if (current_ms - _last_metrics_timestamp_ms >= 1000) {
        HARIS_LOG_DEBUG("Frequency: {} Hz", _accumulated_packet_count);
        _accumulated_packet_count  = 0;
        _last_metrics_timestamp_ms = current_ms;
    }
}

bool PipeServer::accept_client() {
    HARIS_LOG_INFO("Open Request Pipe, wait any client request");

    // Block wait command from client
    int req_read_fd = open(_upstream_path.c_str(), O_RDONLY);
    HARIS_LOG_INFO("Get req_read_fd : {} ", req_read_fd);

    int req_write_fd = -1;
    if (_modes & Ipc::Server::Feedback) {
        req_write_fd = open(_downstream_path.c_str(), O_WRONLY);
    }

    PacketHeader         header;
    std::vector<uint8_t> data;
    bool                 is_new = receive_packet(req_read_fd, header, data);

    // Read data from client request
    if (is_new) {
        if (header.type == DataType::Command && data.size() == sizeof(IPCRequestPayload)) {
            IPCRequestPayload req;
            std::memcpy(&req, data.data(), sizeof(IPCRequestPayload));

            std::string client_id(req.client_id, strnlen(req.client_id, sizeof(req.client_id)));
            HARIS_LOG_INFO("Get client request: {} ", client_id);

            {
                // Check ID has existed:
                std::lock_guard<std::mutex> lock(_client_registry_mutex);

                if (_client_registry.find(client_id) != _client_registry.end()) {
                    HARIS_LOG_DEBUG("Founded Client {} in the container", client_id);
                    // ================= SHIFT LOGIC: PROCESS COMMAND REMOVE =================
                    if (req.command == 2) {
                        HARIS_LOG_DEBUG("Get REMOVE (command = 2) request from client: {} ", client_id);

                        if (_modes & Ipc::Server::Feedback && req_write_fd != -1) {
                            std::string_view reject_msg = "REMOVED";
                            send_packet(req_write_fd, DataType::Command, reject_msg, header.sequence_id);
                        }

                        // 1. Close all File Descriptors for this remove request for this Client
                        int dyn_read_fd  = _client_registry[client_id].first;
                        int dyn_write_fd = _client_registry[client_id].second;

                        if (dyn_read_fd != -1) close(dyn_read_fd);
                        if (dyn_write_fd != -1) close(dyn_write_fd);

                        // 2. Remove all FIFO file in the OS.
                        std::string dynamic_main_path = "/tmp/pipe_" + client_id;
                        std::string dynamic_fb_path   = dynamic_main_path + "_fb";
                        unlink(dynamic_main_path.c_str());
                        unlink(dynamic_fb_path.c_str());

                        // 3. Remove element of the map.
                        _client_registry.erase(client_id);
                        HARIS_LOG_DEBUG("Removed successfully Client");

                        // Clean up Request Pipe of this command.
                        close(req_read_fd);
                        if (req_write_fd != -1) close(req_write_fd);
                        return false;
                    }
                    // =================================================================

                    HARIS_LOG_ERROR("Client ID have been dupplicated request !");
                    HARIS_LOG_ERROR("REJECT new Client: {} ", client_id);

                    // If The request dupplicated a id (REJECTED) for Client
                    if (_modes & Ipc::Server::Feedback && req_write_fd != -1) {
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
            if (_modes & Ipc::Server::Feedback && req_write_fd != -1) {
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
                std::lock_guard<std::mutex> lock(_client_registry_mutex);
                // Store the pair File Descriptors for that Client ID to monitor data.
                _client_registry[client_id] = {dyn_read_fd, dyn_write_fd};
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

    HARIS_LOG_DEBUG("=== read_fd === [{}] ", read_fd);

    // 1. Extract packet header and payload
    if (receive_packet(read_fd, header, data)) {
        monitor_throughput(header.timestamp_ms);

        // 2. Validate payload size
        if (data.empty()) {
            HARIS_LOG_DEBUG("Data: [Empty payload]");
        } else {
            // 3. Forward data to the corresponding handler
            _dispatcher.dispatch(header, data);
        }

        // 4. Send acknowledgment feedback if required
        if (_modes & Ipc::Server::Feedback) {
            IPCRequestPayload reqaaa{"client_id", 33};
            send_packet(write_fd, DataType::Command, reqaaa, header.sequence_id);
        }
    }
}

void PipeServer::dispatch_events() {
    // 1. Acceptor thread remains unchanged
    std::thread acceptor_thread([this]() {
        while (true) {
            if (!accept_client()) std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    acceptor_thread.detach();

    // 2. Efficient polling using OS poll()
    while (true) {
        std::vector<struct pollfd> poll_fds;
        std::vector<std::string>   client_ids;  // Map back poll index to client_id

        // Lock registry only to take a snapshot of active file descriptors
        {
            std::lock_guard<std::mutex> lock(_client_registry_mutex);
            for (const auto& [client_id, fds] : _client_registry) {
                int read_fd = fds.first;
                if (read_fd != -1) {
                    struct pollfd pfd;
                    pfd.fd      = read_fd;
                    pfd.events  = POLLIN;  // Monitor read events
                    pfd.revents = 0;

                    poll_fds.push_back(pfd);
                    client_ids.push_back(client_id);
                }
            }
        }

        // If no clients are connected, sleep briefly to save CPU and retry
        if (poll_fds.empty()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        // Wait up to 100ms for ANY client to send data (OS manages this, 0% CPU overhead)
        int poll_ret = poll(poll_fds.data(), poll_fds.size(), 1);

        if (poll_ret > 0) {
            // Data is available! Loop through and process ONLY active clients
            for (std::size_t i = 0; i < poll_fds.size(); ++i) {
                if (poll_fds[i].revents & POLLIN) {
                    std::string client_id = client_ids[i];
                    int         read_fd   = poll_fds[i].fd;
                    int         write_fd  = -1;

                    // Retrieve write_fd safely from registry
                    {
                        std::lock_guard<std::mutex> lock(_client_registry_mutex);
                        if (_client_registry.find(client_id) != _client_registry.end()) {
                            write_fd = _client_registry[client_id].second;
                        }
                    }

                    // Process packet (this read won't block because OS guaranteed data is ready)
                    if (write_fd != -1) {
                        static uint32_t counter = 0;
                        counter++;
                        HARIS_LOG_TRACE("Client ID: {} - counter {} ", client_id, counter);
                        process_client_packet(client_id, read_fd, write_fd);
                    }
                }

                // Optional: Handle disconnected clients if poll returns POLLHUP / POLLERR
                if (poll_fds[i].revents & (POLLHUP | POLLERR)) {
                    // Handle client disconnection logic here if needed
                }
            }
        } else if (poll_ret < 0) {
            // Handle poll error (except interrupted system call EINTR)
            if (errno != EINTR) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        // No need to sleep_for(3ms) anymore because poll() naturally blocks
        // without consuming CPU when there's no data.
    }
}

}  // namespace HarisLinux
