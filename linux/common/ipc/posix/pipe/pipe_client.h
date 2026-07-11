#pragma once

#include "posix_pipe.h"

namespace HarisLinux {

class PipeClient : public PosixPipe<Ipc::Client> {
 private:
    DECLARE_LOGGER;
    uint32_t _current_seq = 0;
    uint32_t _lost_count  = 0;

    SharedFileDescriptor<PipePolicy> _write_share_fd;
    SharedFileDescriptor<PipePolicy> _read_share_fd;

    std::string _client_id;
    std::string _old_upstream_path;

    /** @brief Helper support cast data type of message. */
    PacketDispatcher<DataHandlerPolicy> _dispatcher{"PipeClient"};

 public:
    PipeClient(const std::string& request_path, const std::string& id, Ipc::Generic<Ipc::Client> modes);
    virtual ~PipeClient();
    void check_feedback();
    bool request_and_switch_main_pipe();
    bool request_and_switch_pipe(uint32_t command_arg = 1);

    template <typename T>
    void push_data(DataType type, const T& data) {
        _current_seq++;
        send_packet(_write_share_fd, type, data, _current_seq);  // call Send in base class
    }
};

}  // namespace HarisLinux
