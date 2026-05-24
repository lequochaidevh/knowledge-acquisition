#pragma once

#include "std17pch.h"

namespace HarisLinux {

enum class DataType : uint8_t {
    Number,
    Text,
    Command,
    Media,
    Heartbeat  // check connected (Lose From Feedback)
};

struct __attribute__((packed)) PacketHeader {
    DataType type;          // 1 byte
    uint32_t payload_size;  // 4 bytes
    uint64_t timestamp_ms;  // 8 bytes // calc Hz
    uint32_t sequence_id;   // 4 bytes // check lose data
};

struct __attribute__((packed)) PipeRequestPayload {
    char     client_id[32];  // to naming for pipe
    uint32_t command;        // Command: 1 = Register, 2 = Disconnect)
};

class PosixPipe {
 protected:
    int         _read_fd  = -1;
    int         _write_fd = -1;
    std::string _pipe_path_main;
    std::string _pipe_path_fb;  // feedback pipe

    uint64_t get_current_time_ms();

 public:
    PosixPipe(const std::string& path) : _pipe_path_main(path), _pipe_path_fb(path + "_fb") {}
    virtual ~PosixPipe() {
        if (_read_fd != -1) close(_read_fd);
        if (_write_fd != -1) close(_write_fd);
    }

    // API send data: data acquisition
    template <typename T>
    bool send_packet(int fd, DataType type, const T& data, uint32_t seq = 0) {
        if (fd == -1) return false;

        const uint8_t* ptr  = reinterpret_cast<const uint8_t*>(data.data());
        uint32_t       size = static_cast<uint32_t>(data.size() * sizeof(typename T::value_type));

        PacketHeader header{type, size, get_current_time_ms(), seq};

        if (write(fd, &header, sizeof(header)) != sizeof(header)) return false;
        if (size > 0 && write(fd, ptr, size) != static_cast<ssize_t>(size)) return false;
        return true;
    }

    bool receive_packet(int fd, PacketHeader& header, std::vector<uint8_t>& payload);
};

}  // namespace HarisLinux