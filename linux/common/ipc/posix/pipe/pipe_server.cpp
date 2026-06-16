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

    // --- HIGH-PRECISION TIMEOUT HANDLING USING SELECT (MICROSECONDS) ---
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(read_fd, &read_fds);  // Add read_fd to the monitoring set

    // Set timeout duration in microseconds (e.g., 500,000 µs = 500 ms = 0.5 seconds)
    struct timeval timeout;
    timeout.tv_sec  = 0;    // Seconds
    timeout.tv_usec = 888;  // Microseconds (µs)

    // Block efficiently until data arrives or timeout expires (0% CPU usage)
    // First argument must be (highest fd + 1)
    int ret = select(read_fd + 1, &read_fds, NULL, NULL, &timeout);

    if (ret < 0) {
        // System error occurred during select execution
        if (errno == EINTR) return;  // Interrupted by system signal, safe to skip
        HARIS_LOG_ERROR("Select error: {}", errno);
        return;
    } else if (ret == 0) {
        // Timeout expired with no incoming data
        return;  // Exit to prevent blocking on read operations
    }

    // Data is ready to be read from the file descriptor
    if (FD_ISSET(read_fd, &read_fds)) {
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
}

void PipeServer::dispatch_events() {
    // Create thread because it will block at open pipe
    std::thread acceptor_thread([this]() {
        while (true) {
            // auto block to wait new client, and add it to <_client_registry> container
            if (!accept_client())
                // wait OS clean up
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    acceptor_thread.detach();  // Trigger thread

    // polling all client connected but non-block read
    while (true) {
        // Loop all element in map: _client_registry
        // Key: client_id, Value: std::pair<int, int> contain {read_fd, write_fd}
        {
            std::lock_guard<std::mutex> lock(_client_registry_mutex);
            for (const auto& [client_id, fds] : _client_registry) {
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
