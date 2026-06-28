#include "pipe_server.h"
namespace HarisLinux {

PipeServer::PipeServer(const std::string& path, Ipc::Generic<Ipc::Server> modes)
    :  //
      PosixPipe<Ipc::Server>(path, modes) {
    INIT_LOGGER("PipeServer");
    logger->setLevel(LogLevel::Trace);

    UniqueFileDescriptor::create_fifo_direction(_upstream_path);
    if (_modes & Ipc::Server::Feedback) {
        UniqueFileDescriptor::create_fifo_direction(_downstream_path);
    }
    HARIS_LOG_INFO("Request Pipe available...");
}

PipeServer::~PipeServer() {}

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
    UniqueFileDescriptor req_read_smart(open(_upstream_path.c_str(), O_RDONLY), FileType::Generic);
    HARIS_LOG_INFO("Get req_read_fd : {} ", req_read_smart.get());

    UniqueFileDescriptor req_write_smart;
    if (_modes & Ipc::Server::Feedback) {
        req_write_smart = UniqueFileDescriptor(open(_downstream_path.c_str(), O_WRONLY), FileType::Generic);
    }

    PacketHeader         header;
    std::vector<uint8_t> data;
    bool                 is_new = receive_packet(req_read_smart.get(), header, data);

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

                        if (_modes & Ipc::Server::Feedback && req_write_smart.is_valid()) {
                            std::string_view reject_msg = "REMOVED";
                            send_packet(req_write_smart.get(), DataType::Command, reject_msg, header.sequence_id);
                        }

                        // 1. Close all File Descriptors for this remove request for this Client
                        _client_registry.erase(client_id);

                        HARIS_LOG_DEBUG("Removed successfully Client");

                        return false;
                    }
                    // =================================================================

                    HARIS_LOG_ERROR("Client ID have been dupplicated request !");
                    HARIS_LOG_ERROR("REJECT new Client: {} ", client_id);

                    // If The request dupplicated a id (REJECTED) for Client
                    if (_modes & Ipc::Server::Feedback && req_write_smart.is_valid()) {
                        std::string_view reject_msg = "REJECTED";
                        send_packet(req_write_smart.get(), DataType::Command, reject_msg, header.sequence_id);
                    }

                    return false;
                }
            }

            // 1. Get ID and Create pipe for that client.
            std::string dynamic_main_path = "/tmp/pipe_" + client_id;
            std::string dynamic_fb_path   = dynamic_main_path + "_fb";

            UniqueFileDescriptor::create_fifo_direction(dynamic_main_path);
            UniqueFileDescriptor::create_fifo_direction(dynamic_fb_path);

            // 2. Send ACK to notify for the client "the pipe have been created successfully"
            if (_modes & Ipc::Server::Feedback && req_write_smart.is_valid()) {
                std::string_view ack_msg = "OPENED";
                send_packet(req_write_smart.get(), DataType::Command, ack_msg, header.sequence_id);
            }

            // Acquire client dedicated stream assets securely using our automated factory methods
            UniqueFileDescriptor smart_read_fd =  //
                UniqueFileDescriptor::create_fifo_fd(dynamic_main_path, O_RDONLY | O_NONBLOCK);
            // block
            UniqueFileDescriptor smart_write_fd =  //
                UniqueFileDescriptor::create_fifo_fd(dynamic_fb_path, O_WRONLY);

            // Explicit Resource Refresh: Assign empty envelopes to trigger instant cleanup of request channels
            req_read_smart  = UniqueFileDescriptor();
            req_write_smart = UniqueFileDescriptor();

            // 4. Open new pipe (server - client_id)
            HARIS_LOG_DEBUG("Wait client join the /tmp/pipe_{} pipe ...", client_id);
            {
                std::lock_guard<std::mutex> lock(_client_registry_mutex);
                // Package and move the live smart objects directly into the database container safely
                _client_registry[client_id] =  //
                    std::make_pair(std::move(smart_read_fd), std::move(smart_write_fd));
            }
            HARIS_LOG_DEBUG("Connected to client successfully: {} ", client_id);

            // Test stop server signal
            if (client_id == "client_alpha_9999") {
                HARIS_LOG_INFO("Stop server when meet client {} ", client_id);
                _server_running = false;
            }
            return true;
        }
    }

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
            HARIS_LOG_ERROR("Data: [Empty payload]");
        } else {
            // 3. Forward data to the corresponding handler
            _dispatcher.dispatch(header, data);
        }

        // 4. Send acknowledgment feedback if required
        if (_modes & Ipc::Server::Feedback) {
            IPCRequestPayload fb_payload{"client_id", 33};
            send_packet(write_fd, DataType::Command, fb_payload, header.sequence_id);
        }
    }
}

void PipeServer::dispatch_events() {
    // 1. Acceptor thread remains unchanged
    std::thread acceptor_thread([this]() {
        while (_server_running) {
            if (!accept_client()) std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });
    acceptor_thread.detach();

    // 2. Efficient polling using OS poll()
    while (_server_running) {
        std::vector<struct pollfd> poll_fds;
        std::vector<std::string>   client_ids;  // Map back poll index to client_id

        // Lock registry only to take a snapshot of active file descriptors
        {
            std::lock_guard<std::mutex> lock(_client_registry_mutex);
            for (const auto& [client_id, fds] : _client_registry) {
                int read_fd = fds.first.get();
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
                            write_fd = _client_registry[client_id].second.get();
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
