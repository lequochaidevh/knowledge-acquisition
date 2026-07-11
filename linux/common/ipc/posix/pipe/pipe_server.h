#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

/**
 * @brief Autonomous class managing dynamic client states, I/O channels, and performance calculations.
 * @details Encapsulates telemetry monitoring metrics like heartbeat and packet throughput within a private scope.
 */
class ClientSession {
 private:
    // Directional communication channels managed via smart reference lifecycles
    SharedFileDescriptor<PipePolicy> _read_fd{};
    SharedFileDescriptor<PipePolicy> _write_fd{};

    // Localized synchronization primitive protecting internal state calculations
    mutable std::mutex _session_mutex{};

    /* Core performance telemetry metrics tracking counters with private prefixing */
    uint64_t _last_heartbeat_timestamp_ms  = 0;    ///< Epoch time tracking the last valid keep-alive token.
    uint64_t _last_metrics_timestamp_ms    = 0;    ///< Epoch time of the last window evaluation.
    uint32_t _accumulated_packet_count     = 0;    ///< Total packets processed in the active sampling interval.
    double   _current_message_frequency_hz = 0.0;  ///< Computed message frequency throughput in Hertz.

 public:
    /**
     * @brief Explicit initialization constructor for zeroheap setup execution.
     */
    ClientSession() noexcept = default;

    ClientSession(SharedFileDescriptor<PipePolicy> r_fd, SharedFileDescriptor<PipePolicy> w_fd) noexcept
        : _read_fd(std::move(r_fd)), _write_fd(std::move(w_fd)) {}

    /**
     * @brief Explicitly defined Move Constructor to bypass the non-copyable std::mutex constraint.
     */
    ClientSession(ClientSession&& other) noexcept {
        // Enforce strict lock synchronization on the source object before moving states
        std::lock_guard<std::mutex> lock(other._session_mutex);

        _read_fd                      = std::move(other._read_fd);
        _write_fd                     = std::move(other._write_fd);
        _last_heartbeat_timestamp_ms  = other._last_heartbeat_timestamp_ms;
        _last_metrics_timestamp_ms    = other._last_metrics_timestamp_ms;
        _accumulated_packet_count     = other._accumulated_packet_count;
        _current_message_frequency_hz = other._current_message_frequency_hz;
    }

    /**
     * @brief Explicitly defined Move Assignment Operator to bypass the non-copyable std::mutex constraint.
     * @details This directly satisfies the structural mapping operator requirement inside std::unordered_map buckets.
     */
    ClientSession& operator=(ClientSession&& other) noexcept {
        if (this != &other) {
            // Apply scoped atomic locks on both instances to prevent cross-thread race conditions during allocation
            std::scoped_lock lock(_session_mutex, other._session_mutex);

            _read_fd                      = std::move(other._read_fd);
            _write_fd                     = std::move(other._write_fd);
            _last_heartbeat_timestamp_ms  = other._last_heartbeat_timestamp_ms;
            _last_metrics_timestamp_ms    = other._last_metrics_timestamp_ms;
            _accumulated_packet_count     = other._accumulated_packet_count;
            _current_message_frequency_hz = other._current_message_frequency_hz;
        }
        return *this;
    }

    // Explicitly delete Copy operations to enforce strict object resource protection guidelines
    ClientSession(const ClientSession&) = delete;
    ClientSession& operator=(const ClientSession&) = delete;

    /**
     * @brief Thread-safe getter to access the private write file description reference.
     * @return SharedFileDescriptor<PipePolicy>& Reference to the underlying write descriptor channel.
     */
    const SharedFileDescriptor<PipePolicy>& get_write_sfd() const noexcept { return _write_fd; }

    /**
     * @brief Thread-safe getter to access the private read file description reference.
     * @return SharedFileDescriptor<PipePolicy>& Reference to the underlying read descriptor channel.
     */
    const SharedFileDescriptor<PipePolicy>& get_read_sfd() const noexcept { return _read_fd; }

    /**
     * @brief Thread-safe getter to retrieve the current message processing frequency.
     * @return double The message transmission frequency in Hertz.
     */
    double get_message_frequency() const noexcept {
        std::lock_guard<std::mutex> lock(_session_mutex);
        return _current_message_frequency_hz;
    }

    /**
     * @brief Updates the heartbeat tracker registry with the latest systemic time.
     * @param current_time_ms The runtime system clock checkpoint in milliseconds.
     */
    void register_heartbeat(uint64_t current_time_ms) noexcept {
        std::lock_guard<std::mutex> lock(_session_mutex);
        _last_heartbeat_timestamp_ms = current_time_ms;
    }

    /**
     * @brief Records a processed packet and re-evaluates the runtime message transmission frequency.
     * @param current_time_ms The runtime system clock checkpoint in milliseconds.
     */
    void evaluate_message_frequency(uint64_t current_time_ms) noexcept {
        std::lock_guard<std::mutex> lock(_session_mutex);
        _accumulated_packet_count++;

        uint64_t time_delta_ms = current_time_ms - _last_metrics_timestamp_ms;
        if (time_delta_ms >= 1000) {  // Evaluate frequency once every 1-second sampling window
            // Convert milliseconds interval boundary into a frequency calculation (Hz)
            _current_message_frequency_hz = (static_cast<double>(_accumulated_packet_count) * 1000.0) / time_delta_ms;

            // Reset window counters to initiate the subsequent measurement stage
            _accumulated_packet_count  = 0;
            _last_metrics_timestamp_ms = current_time_ms;
        }
    }

    /**
     * @brief Verifies whether the client has breached the configured heartbeat tolerance window.
     * @param current_time_ms The runtime system clock checkpoint in milliseconds.
     * @param timeout_limit_ms The maximum permitted time gap before declaring connection loss.
     * @return true If the client is dead, false if the connection remains alive.
     */
    bool is_heartbeat_expired(uint64_t current_time_ms, uint64_t timeout_limit_ms) const noexcept {
        std::lock_guard<std::mutex> lock(_session_mutex);

        if (_last_heartbeat_timestamp_ms == 0) return false;  // Avoid false positives before initialization
        return (current_time_ms - _last_heartbeat_timestamp_ms) > timeout_limit_ms;
    }
};

class PipeServer : public PosixPipe<Ipc::Server> {
 private:
    DECLARE_LOGGER;
    std::string _server_id = "server_name";
    // Registry managing the object-oriented ClientSession tracking foot-print
    std::unordered_map<std::string, ClientSession> _client_registry;
    std::mutex                                     _client_registry_mutex;

    bool _server_running = true;
    /** @brief Helper support cast data type of message. */
    PacketDispatcher<DataHandlerPolicy> _dispatcher{"PipeServer"};

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
    void process_client_packet(const std::string& client_id, SharedFileDescriptor<PipePolicy>& proxy_read_fd,
                               SharedFileDescriptor<PipePolicy>& proxy_write_fd);

    void execute_housekeeping_phase();

 public:
    PipeServer(const std::string& path, Ipc::Generic<Ipc::Server> modes);

    ~PipeServer();

    bool is_server_running() { return _server_running; }

    void dispatch_events();
};

}  // namespace HarisLinux
