#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeServer : public PosixPipe {
 public:
    enum class ServerMode { ReadOnly, Feedback, Broadcast };

 private:
    ServerMode mode;

    // Calc Hz
    uint64_t last_time = 0;
    uint32_t pkt_count = 0;

    void CalculateHz(uint64_t current_ms) {
        pkt_count++;
        if (last_time == 0) last_time = current_ms;
        if (current_ms - last_time >= 1000) {
            std::cout << "[Server] Frequency: " << pkt_count << " Hz\n";
            pkt_count = 0;
            last_time = current_ms;
        }
    }

 public:
    PipeServer(const std::string& path, ServerMode m) : PosixPipe(path), mode(m) {
        // unlink(pipe_path_main.c_str());
        // unlink(pipe_path_fb.c_str());
        mkfifo(pipe_path_main.c_str(), 0666);
        if (mode == ServerMode::Feedback) {
            mkfifo(pipe_path_fb.c_str(), 0666);
        }
    }

    void Start();

    void ProcessIncomingTL();

    void ProcessIncoming();
};

}  // namespace HarisLinux