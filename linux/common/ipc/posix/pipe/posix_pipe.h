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

class PosixPipe {
 protected:
    int         read_fd  = -1;
    int         write_fd = -1;
    std::string pipe_path_main;
    std::string pipe_path_fb;  // feedback pipe

 public:
    PosixPipe(const std::string& path) : pipe_path_main(path), pipe_path_fb(path + "_fb") {}
    virtual ~PosixPipe() {
        if (read_fd != -1) close(read_fd);
        if (write_fd != -1) close(write_fd);
    }

    // API send data: data acquisition
    bool SendPacket(int fd, DataType type, const std::vector<uint8_t>& data, uint32_t seq = 0);

    bool ReadPacket(int fd, PacketHeader& header, std::vector<uint8_t>& data);
};

}  // namespace HarisLinux