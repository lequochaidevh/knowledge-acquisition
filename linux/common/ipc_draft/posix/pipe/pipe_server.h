#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeServer : public PosixPipe {
 public:
    enum class ServerMode { ReadOnly, Feedback, Broadcast };

 private:
    ServerMode _mode;

    std::map<std::string, std::pair<int, int>> _active_clients;
    std::mutex                                 _active_clients_mutex;

    // calc hz
    uint64_t _last_time = 0;
    uint32_t _pkt_count = 0;

    void calculate_hz(uint64_t current_ms);

    bool accept_client();

    void process_client_packet(const std::string& client_id, int read_fd, int write_fd);

 public:
    PipeServer(const std::string& path, ServerMode m);

    ~PipeServer();

    void poll_all_clients();
};

}  // namespace HarisLinux
