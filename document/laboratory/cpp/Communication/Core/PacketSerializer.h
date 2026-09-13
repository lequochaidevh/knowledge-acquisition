#pragma once
#include "Packet.h"
#include "ByteUtilities.h"
#include <vector>
#include <cstdint>

class PacketSerializer {
 public:
    PacketSerializer() = default;

    // Serializes a clean high-level Packet structure into a raw binary frame
    // Frame layout matches ComlinkParser rules:
    // [0xAA] [SystemIdentifier] [MessageIdentifier_High] [MessageIdentifier_Low] [Length] [Payload...] [Checksum]
    static std::vector<uint8_t> serialize(const Packet& packet) {
        std::vector<uint8_t> frame;
        frame.reserve(6 + packet.payload.size());

        // 1. Write preamble start delimiter (STX)
        frame.push_back(0xAA);

        // 2. Write tracking parameters
        frame.push_back(packet.system_id);

        // 3. Deconstruct 16-bit message identifier into high and low byte sequences
        frame.push_back(static_cast<uint8_t>((packet.msg_id >> 8) & 0xFF));
        frame.push_back(static_cast<uint8_t>(packet.msg_id & 0xFF));

        // 4. Write variable payload bound size
        frame.push_back(static_cast<uint8_t>(packet.payload.size()));

        // 5. Append data segment block
        frame.insert(frame.end(), packet.payload.begin(), packet.payload.end());

        // 6. Compute real-time frame checksum including payload elements
        uint16_t computed_checksum = ByteUtilities::calculate_checksum(frame.data(), frame.size());

        // 7. Append checksum indicator byte (extract low byte matching parser rules)
        frame.push_back(static_cast<uint8_t>(computed_checksum & 0xFF));

        return frame;
    }

    // Advanced Helper: Pack a direct custom C++ struct into a target dynamic Packet wrapper
    template <typename T>
    static Packet pack_struct(uint8_t system_id, uint16_t msg_id, const T& custom_struct) {
        Packet packet;
        packet.system_id = system_id;
        packet.msg_id    = msg_id;

        packet.payload.resize(sizeof(T));
        ByteUtilities::serialize_type(packet.payload.data(), custom_struct);

        return packet;
    }
};
