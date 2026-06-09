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

namespace Ipc {
    // --- SERVER FLAGS ---
    enum class Server : uint8_t { ReadOnly = 0x00, Feedback = 0x01, CheckLose = 0x02, Broadcast = 0x04 };

    inline Server operator|(Server a, Server b) {
        return static_cast<Server>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    inline Server& operator|=(Server& a, Server b) {
        a = a | b;
        return a;
    }

    inline bool operator&(Server a, Server b) { return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0; }

    // --- CLIENT FLAGS ---
    enum class Client : uint8_t { Send = 0x00, Feedback = 0x10, CheckLose = 0x20 };

    // OR
    inline Client operator|(Client a, Client b) {
        return static_cast<Client>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    // |=
    inline Client& operator|=(Client& a, Client b) {
        a = a | b;
        return a;
    }

    inline bool operator&(Client a, Client b) { return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0; }
}  // namespace Ipc

}  // namespace HarisLinux