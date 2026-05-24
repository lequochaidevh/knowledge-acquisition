#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeClient : public PosixPipe {
 public:
    enum class ClientMode { Send, CheckLose };

 private:
    ClientMode _mode;
    uint32_t   _current_seq = 0;
    uint32_t   _lost_count  = 0;

 public:
    PipeClient(const std::string& path, ClientMode m) : PosixPipe(path), _mode(m) {}

    void connect();

    void check_feedback();

    template <typename T>
    void PushData(DataType type, const T& data) {
        _current_seq++;
        send_packet(_write_fd, type, data, _current_seq);  // call Send in base class
    }
};

}  // namespace HarisLinux