#pragma once
#include "std17pch.h"

namespace HarisLinux {

enum class DataType : uint8_t { Number, Text, Command, Media, Heartbeat };

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

// Define Server behaviors as distinct binary bit fields
enum IPC_SERVER_FLAGS : uint8_t {
    IPC_SERVER_READ_ONLY  = 0x00,  // 0000 0000 -> Silent consumer
    IPC_SERVER_FEEDBACK   = 0x01,  // 0000 0001 -> Mirrors incoming packets back
    IPC_SERVER_CHECK_LOSE = 0x02,  // 0000 0100 -> Outbound packet sequence tracking
    IPC_SERVER_BROADCAST  = 0x04   // 0000 0010 -> Sends to all discovered nodes
};

// Define Client behaviors as distinct binary bit fields
enum IPC_CLIENT_FLAGS : uint8_t {
    IPC_CLIENT_SEND       = 0x00,  // 0000 0000 -> Pure low-overhead sender
    IPC_CLIENT_FEEDBACK   = 0x01,  // 0000 0001 -> Mirrors server commands back
    IPC_CLIENT_CHECK_LOSE = 0x02   // 0000 0010 -> Tracks its own sent sequences
};

}  // namespace HarisLinux