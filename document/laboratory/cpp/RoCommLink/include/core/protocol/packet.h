#pragma once
#include "std17pch.h"

struct Packet {
    uint8_t              system_id = 0;
    uint16_t             msg_id    = 0;
    std::vector<uint8_t> payload;
    uint16_t             checksum = 0;
};
