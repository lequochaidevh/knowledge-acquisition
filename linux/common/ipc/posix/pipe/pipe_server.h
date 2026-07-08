#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeServer : public PosixPipe<Ipc::Server> {
 private:
    DECLARE_LOGGER;

    std::map<std::string, std::pair<SharedFileDescription<PipePolicy>, SharedFileDescription<PipePolicy>>>  //
        _client_registry;

    std::mutex _client_registry_mutex;

    bool _server_running = true;

    /* Metrics & Performance Monitoring counters */
    uint64_t _last_metrics_timestamp_ms = 0;  ///< Epoch time of the last throughput evaluation.
    uint32_t _accumulated_packet_count  = 0;  ///< Total packets processed in the current window.

    /** @brief Helper support cast data type of message. */
    PacketDispatcher<DataHandlerPolicy> _dispatcher{"PipeServer"};

    /**
     * @brief Computes and logs the operational throughput (packets per second).
     * @param current_time_ms The current system monotonic clock in milliseconds.
     */
    void monitor_throughput(uint64_t current_time_ms);

    /**
     * @brief Handshakes and establishes connection with a newly requesting client.
     * @return true If the client was successfully authorized and registered.
     * @return false If connection setup failed or encountered an error.
     */
    bool accept_client();

    /**
     * @brief Dispatches and handles a single incoming packet from a specific client channel.
     * @param client_id Unique identifier of the targeting client.
     * @param read_fd The data source descriptor.
     * @param proxy_write_fd The data destination feedback descriptor.
     */
    void process_client_packet(const std::string& client_id, SharedFileDescription<PipePolicy>& proxy_read_fd,
                               SharedFileDescription<PipePolicy>& proxy_write_fd);

 public:
    PipeServer(const std::string& path, Ipc::Generic<Ipc::Server> modes);

    ~PipeServer();

    bool is_server_running() { return _server_running; }

    void dispatch_events();
};

}  // namespace HarisLinux
