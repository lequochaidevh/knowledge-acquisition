#pragma once
#include "std17pch.h"

enum class CommandResult : uint8_t { ACCEPTED = 0, TEMPORARILY_REJECTED = 1, DENIED = 2, FAILED = 3 };

#pragma pack(push, 1)
struct PacketHeader {
    uint8_t  magic        = 0x5A;
    uint8_t  version      = 1;
    uint8_t  system_id    = 0;
    uint8_t  component_id = 0;
    uint16_t msg_id       = 0;
    uint16_t sequence     = 0;
    uint16_t payload_len  = 0;
};
#pragma pack(pop)

struct Packet {
    PacketHeader         header;
    std::vector<uint8_t> payload;
    uint16_t             checksum = 0;
};

#pragma pack(push, 1)
struct CommandAckPayload {
    uint16_t command_msg_id;
    uint16_t target_sequence;
    uint8_t  result;
};
#pragma pack(pop)