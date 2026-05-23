#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeServer : public PosixPipe {
 public:
    enum class ServerMode { ReadOnly, Feedback, Broadcast };

 private:
    ServerMode mode;

 public:
    PipeServer(const std::string& path, ServerMode m) : PosixPipe(path), mode(m) {
        mkfifo(pipe_path_main.c_str(), 0666);
        if (mode == ServerMode::Feedback) {
            mkfifo(pipe_path_fb.c_str(), 0666);
        }
    }

    void Start();

    void ProcessIncoming();
};

}  // namespace HarisLinux