#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeClient : public PosixPipe {
 public:
    enum class ClientMode { Send, CheckLose };

 private:
    ClientMode mode;
    uint32_t   current_seq = 0;

 public:
    PipeClient(const std::string& path, ClientMode m) : PosixPipe(path), mode(m) {}

    void Connect();

    void SendData(DataType type, const std::vector<uint8_t>& data);

    void CheckFeedback();
};

}  // namespace HarisLinux