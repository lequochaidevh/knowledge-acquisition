#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeServer : public PosixPipe {
 public:
    enum class ServerMode { ReadOnly, Feedback, Broadcast };

 private:
    ServerMode _mode;

    // calc hz
    uint64_t _last_time = 0;
    uint32_t _pkt_count = 0;

    void calculate_hz(uint64_t current_ms) {
        _pkt_count++;
        if (_last_time == 0) _last_time = current_ms;
        if (current_ms - _last_time >= 1000) {
            std::cout << "[Server] Frequency: " << _pkt_count << " Hz\n";
            _pkt_count = 0;
            _last_time = current_ms;
        }
    }

 public:
    PipeServer(const std::string& path, ServerMode m) : PosixPipe(path), _mode(m) {
        // unlink(pipe_path_main.c_str());
        // unlink(pipe_path_fb.c_str());
        mkfifo(_pipe_path_main.c_str(), 0666);
        if (_mode == ServerMode::Feedback) {
            mkfifo(_pipe_path_fb.c_str(), 0666);
        }
    }

    void start();

    void process_data_incoming();
};

}  // namespace HarisLinux