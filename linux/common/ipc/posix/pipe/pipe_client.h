#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeClient : public PosixPipe {
 public:
    enum class ClientMode { Send, CheckLose };

 private:
    ClientMode mode;
    uint32_t   current_seq = 0;
    uint32_t   lost_count  = 0;

 public:
    PipeClient(const std::string& path, ClientMode m) : PosixPipe(path), mode(m) {}

    void Connect();

    void SendData(DataType type, const std::vector<uint8_t>& data);

    void CheckFeedback();

    template <typename T>
    void PushData(DataType type, const T& data) {
        current_seq++;
        Send(write_fd, type, data, current_seq);  // call Send in base class
    }
};

}  // namespace HarisLinux