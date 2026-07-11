#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeClient : public PosixPipe<Ipc::Client> {
 private:
    DECLARE_LOGGER;
    uint32_t   _current_seq = 0;
    uint32_t   _lost_count  = 0;
    std::mutex _current_seq_sync;

    SharedFileDescriptor<PipePolicy> _write_share_fd;
    SharedFileDescriptor<PipePolicy> _read_share_fd;

    std::string _client_id;
    std::string _old_upstream_path;

    std::atomic<bool> _is_running{false};

    /** @brief Helper support cast data type of message. */
    PacketDispatcher<DataHandlerPolicy> _dispatcher{"PipeClient"};

 public:
    PipeClient(const std::string& request_path, const std::string& id, Ipc::Generic<Ipc::Client> modes);
    virtual ~PipeClient();
    void check_feedback();
    void start_monitoring();
    void stop_monitoring();
    bool request_and_switch_main_pipe();
    bool request_and_switch_pipe(uint32_t command_arg = 1);

    bool push_heartbeat() {
        std::lock_guard<std::mutex> lock(_current_seq_sync);
        _current_seq++;
        return send_heartbeat(_write_share_fd, _client_id, _current_seq);
    }

    template <typename T>
    bool push_data(DataType type, const T& data) {
        std::lock_guard<std::mutex> lock(_current_seq_sync);
        _current_seq++;
        return send_packet(_write_share_fd, type, data, _current_seq);  // call Send in base class
    }
};

}  // namespace HarisLinux
