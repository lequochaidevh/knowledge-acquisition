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

    std::string _client_id;
    std::string _old_pipe_path_main;

 public:
    PipeClient(const std::string& request_path, const std::string& id, ClientMode m)
        : PosixPipe(request_path), _old_pipe_path_main(request_path), _client_id(id), _mode(m) {}
    virtual ~PipeClient();
    void check_feedback();
    bool request_and_switch_main_pipe();
    bool request_and_switch_pipe(uint32_t command_arg = 1);

    template <typename T>
    void push_data(DataType type, const T& data) {
        _current_seq++;
        send_packet(_write_fd, type, data, _current_seq);  // call Send in base class
    }
};

}  // namespace HarisLinux