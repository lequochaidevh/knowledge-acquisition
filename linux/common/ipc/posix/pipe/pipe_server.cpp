#include "pipe_server.h"
namespace HarisLinux {

PipeServer::PipeServer(const std::string& path, Ipc::Generic<Ipc::Server> modes)
    :  //
      PosixPipe<Ipc::Server>(path, modes) {
    INITIALIZE_LOGGER_SELF;
    {
        PipeContext pipe_main_ctx{_upstream_path};
        SharedFileDescriptor<PipePolicy>::setup_communication(pipe_main_ctx);
    }
    {
        PipeContext pipe_ctx{_downstream_path};
        SharedFileDescriptor<PipePolicy>::setup_communication(pipe_ctx);
    }

    HARIS_LOG_INFO("Request Pipe available...");
}

PipeServer::~PipeServer() {}

/**
 * @brief Periodic background task executed on the main server runtime loop.
 */
void PipeServer::execute_housekeeping_phase() {
    uint64_t                    current_time_ms = get_current_timestamp_ms();
    std::lock_guard<std::mutex> lock(_client_registry_mutex);

    // Use a clean iterator to safely erase items while traversing the map
    for (auto it = _client_registry.begin(); it != _client_registry.end();) {
        ClientSession&     session   = it->second;
        const std::string& client_id = it->first;
        // 1. Evaluate client lifecycle expiration via internal calculations
        if (session.is_heartbeat_expired(current_time_ms, 5000)) {  // 5-second deadline limit
            HARIS_LOG_WARN("Client connection lost via heartbeat timeout: {}", client_id);
            it = _client_registry.erase(it);  // Drop session from tracking registry matrix
            continue;
        }

        // 2. Read live data throughput indicators securely through public encapsulation getters
        double throughput_hz = session.get_message_frequency();
        // HARIS_LOG_DEBUG("Telemetry report - Client: {} | Throughput: {:.2f} Hz", client_id, throughput_hz);

        ++it;
    }
}

bool PipeServer::accept_client() {
    HARIS_LOG_INFO("Open Request Pipe, wait any client request");

    // Block wait command from client
    SharedFileDescriptor<PipePolicy> req_read_smart(open(_upstream_path.c_str(), O_RDONLY));
    HARIS_LOG_INFO("Get req_read_fd : {} ", req_read_smart.get());

    SharedFileDescriptor<PipePolicy> req_write_smart;
    if (_modes & Ipc::Server::Feedback) {
        // init with raw fd
        req_write_smart = SharedFileDescriptor<PipePolicy>(open(_downstream_path.c_str(), O_WRONLY));
    }

    PacketHeader         header;
    std::vector<uint8_t> data;
    bool                 is_new = receive_packet(req_read_smart, header, data);

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

                        if (_modes & Ipc::Server::Feedback && req_write_smart) {
                            std::string_view reject_msg = "REMOVED";
                            send_packet(req_write_smart, DataType::Command, reject_msg, header.sequence_id);
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
                    if (_modes & Ipc::Server::Feedback && req_write_smart) {
                        std::string_view reject_msg = "REJECTED";
                        send_packet(req_write_smart, DataType::Command, reject_msg, header.sequence_id);
                    }

                    return false;
                }
            }

            // 1. Get ID and Create pipe for that client.
            std::string dynamic_main_path = "/tmp/pipe_" + client_id;
            std::string dynamic_fb_path   = dynamic_main_path + "_fb";

            {
                PipeContext pipe_main_ctx{dynamic_fb_path};
                SharedFileDescriptor<PipePolicy>::setup_communication(pipe_main_ctx);
            }
            {
                PipeContext pipe_ctx{dynamic_main_path};
                SharedFileDescriptor<PipePolicy>::setup_communication(pipe_ctx);
            }

            // 2. Send ACK to notify for the client "the pipe have been created successfully"
            if (_modes & Ipc::Server::Feedback && req_write_smart) {
                std::string_view ack_msg = "OPENED";
                send_packet(req_write_smart, DataType::Command, ack_msg, header.sequence_id);
            }

            // Acquire client dedicated stream assets securely using our automated factory methods

            SharedFileDescriptor<PipePolicy>  //
                smart_read_fd(open(dynamic_main_path.c_str(), O_RDONLY | O_NONBLOCK));
            // block
            HARIS_LOG_DEBUG("Wait client join the /tmp/pipe_{} pipe ...", client_id);
            SharedFileDescriptor<PipePolicy>                              //
                smart_write_fd(open(dynamic_fb_path.c_str(), O_WRONLY));  //

            // 4. Open new pipe (server - client_id)
            HARIS_LOG_DEBUG("Client have joined");
            {
                std::lock_guard<std::mutex> lock(_client_registry_mutex);
                // Construct the object directly inside the map bucket memory layout to avoid redundant moves
                _client_registry.emplace(std::piecewise_construct,  //
                                         std::forward_as_tuple(client_id),
                                         std::forward_as_tuple(std::move(smart_read_fd),  //
                                                               std::move(smart_write_fd)));
                ClientSession* target_session = &(_client_registry[client_id]);
                target_session->register_heartbeat(get_current_timestamp_ms());
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

void PipeServer::process_client_packet(const std::string& client_id, SharedFileDescriptor<PipePolicy>& proxy_read_fd,
                                       SharedFileDescriptor<PipePolicy>& proxy_write_fd) {
    PacketHeader         header;
    std::vector<uint8_t> data;

    HARIS_LOG_DEBUG("=== read_fd === [{}] ", proxy_read_fd.get());

    ClientSession* target_session = nullptr;
    {
        std::lock_guard<std::mutex> lock(_client_registry_mutex);
        auto                        it = _client_registry.find(client_id);
        if (it == _client_registry.end()) return;
        target_session = &(it->second);
    }

    // 1. Extract packet header and payload
    if (receive_packet(proxy_read_fd, header, data)) {
        uint64_t current_time_ms = get_current_timestamp_ms();

        // 2. Command the smart object to recalculate message frequencies autonomously
        target_session->evaluate_message_frequency(current_time_ms);

        // 3. Reset the keep-alive expiration timeline if this was a heartbeat packet
        if (header.type == DataType::Heartbeat) {
            target_session->register_heartbeat(current_time_ms);
        }

        // 2. Validate payload size
        if (data.empty()) {
            HARIS_LOG_ERROR("Data: [Empty payload]");
        } else {
            // 3. Forward data to the corresponding handler
            HARIS_LOG_INFO("Got data successfully");
        }

        // 4. Send acknowledgment feedback if required
        if (_modes & Ipc::Server::Feedback) {
            uint32_t seq_id = header.sequence_id;

            HARIS_LOG_INFO("seq {}", seq_id);
            IPCRequestPayload fb_payload{"Server feedback to client fd: ",  //
                                         static_cast<uint32_t>(proxy_write_fd.get())};

            send_packet(proxy_write_fd, DataType::Command, fb_payload, seq_id);

            static uint64_t last_heartbeat_time = 0;
            if ((current_time_ms - last_heartbeat_time) > 1000) {
                send_heartbeat(proxy_write_fd, _server_id, seq_id);
                last_heartbeat_time = current_time_ms;
            }
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

    // 2. Efficient polling using OS poll() - MAX: 1KHz/client
    while (_server_running) {
        // Check connection timeout.
        execute_housekeeping_phase();

        std::vector<struct pollfd> poll_fds;
        std::vector<std::string>   client_ids;  // Map back poll index to client_id

        // Lock registry only to take a snapshot of active file descriptors
        {
            std::lock_guard<std::mutex> lock(_client_registry_mutex);
            for (const auto& [client_id, fds] : _client_registry) {
                int read_fd = fds.get_read_sfd().get();
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

        // MAX: 1KHz/client - Wait up to 1ms for ANY client to send data (OS manages this, 0% CPU overhead)
        // Todo: Upgrade
        int poll_ret = ::poll(poll_fds.data(), poll_fds.size(), 1);

        if (poll_ret > 0) {
            // Data is available! Loop through and process ONLY active clients
            for (std::size_t i = 0; i < poll_fds.size(); ++i) {
                if (poll_fds[i].revents & POLLIN) {
                    std::string client_id = client_ids[i];
                    int         read_fd   = poll_fds[i].fd;

                    SharedFileDescriptor<PipePolicy> proxy_write_fd;
                    SharedFileDescriptor<PipePolicy> proxy_read_fd;

                    // Retrieve write_fd safely from registry
                    {
                        std::lock_guard<std::mutex> lock(_client_registry_mutex);
                        if (_client_registry.find(client_id) != _client_registry.end()) {
                            proxy_read_fd  = _client_registry[client_id].get_read_sfd();
                            proxy_write_fd = _client_registry[client_id].get_write_sfd();
                        }
                    }

                    // Process packet (this read won't block because OS guaranteed data is ready)
                    if (proxy_write_fd) {
                        static uint32_t counter = 0;
                        counter++;
                        HARIS_LOG_TRACE("Client ID: {} - counter {} ", client_id, counter);
                        process_client_packet(client_id, proxy_read_fd, proxy_write_fd);
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
