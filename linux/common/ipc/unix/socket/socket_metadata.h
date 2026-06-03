#pragma once
#include "std17pch.h"

namespace HarisLinux {

enum class DataType : uint8_t { Number, Text, Command, Media, Heartbeat };

enum class ServerMode { ReadOnly, Feedback, Broadcast };

struct __attribute__((packed)) PacketHeader {
    DataType type;
    uint32_t payload_size;
    uint64_t timestamp_ms;
    uint32_t sequence_id;
};

struct __attribute__((packed)) SocketRequestPayload {
    char     client_id[32];
    uint32_t command;
};

}  // namespace HarisLinux
