#pragma once
#include "std17pch.h"

namespace HarisLinux {

enum class DataType : uint8_t { Number, Text, Command, Media, Heartbeat };

enum class ServerMode { ReadOnly, Feedback, Broadcast };

struct __attribute__((packed)) PacketHeader {
    DataType type;          // 1 byte
    uint32_t payload_size;  // 4 bytes
    uint64_t timestamp_ms;  // 8 bytes // calc Hz
    uint32_t sequence_id;   // 4 bytes // check lose data
};

struct __attribute__((packed)) IPCRequestPayload {
    char     client_id[32];  // to naming for pipe
    uint32_t command;        // Command: 1 = Register, 2 = Disconnect)
};

}  // namespace HarisLinux