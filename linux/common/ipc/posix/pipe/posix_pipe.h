#pragma once

#include "std17pch.h"
#include "logging/logger.h"

#include "ipc_sender.h"
#include "ipc_receiver.h"

namespace HarisLinux {

template <typename Modes>
class PosixPipe : public StreamSender, public StreamReceiver {
 protected:
    int         _read_fd  = -1;
    int         _write_fd = -1;
    std::string _pipe_path_main;
    std::string _pipe_path_fb;  // feedback pipe

    Ipc::Generic<Modes> _modes;

 public:
    PosixPipe(const std::string& path, Ipc::Generic<Modes> modes)
        : StreamSender(-1),
          StreamReceiver(-1),  // send and receive
          _pipe_path_main(path),
          _pipe_path_fb(path + "_fb"),
          _modes(modes) {}

    virtual ~PosixPipe() {
        if (StreamReceiver::_fd != -1) ::close(StreamReceiver::_fd);
        if (StreamSender::_fd != -1) ::close(StreamSender::_fd);

        if (_read_fd != -1) close(_read_fd);
        if (_write_fd != -1) close(_write_fd);
    }

    void set_read_fd(int r_fd) { StreamReceiver::_fd = r_fd; }
    void set_write_fd(int w_fd) { StreamSender::_fd = w_fd; }

    int get_read_fd() const { return StreamReceiver::_fd; }
    int get_write_fd() const { return StreamSender::_fd; }

    // API send data: data acquisition
    template <typename T>
    bool send_packet(int target_fd, DataType type, const T& data, const uint32_t& seq = 0) {
        // core logic writev
        int old_fd        = StreamSender::_fd;
        StreamSender::_fd = target_fd;

        // (Zero-copy / Atomic)
        bool result = StreamSender::send(type, data, seq);

        StreamSender::_fd = old_fd;
        return result;
    }

    bool receive_packet(int fd, PacketHeader& header, std::vector<uint8_t>& payload);
};

}  // namespace HarisLinux
