#pragma once
#include "packet.h"
#include "common/byte_utilities.h"

class PacketSerializer {
 public:
    PacketSerializer() = default;

    // Serializes a clean high-level Packet structure into a raw binary frame
    // Frame layout matches ComlinkParser rules:
    // [0xAA] [SystemIdentifier] [MessageIdentifier_High] [MessageIdentifier_Low] [Length] [Payload...] [Checksum]
    static std::vector<uint8_t> serialize(const Packet& packet);

    // Advanced Helper: Pack a direct custom C++ struct into a target dynamic Packet wrapper
    template <typename T>
    static Packet pack_struct(uint8_t system_id, uint16_t msg_id, const T& custom_struct) {
        Packet packet;

        packet.header.magic        = 0xAA;
        packet.header.version      = 1;
        packet.header.system_id    = system_id;
        packet.header.component_id = 0;
        packet.header.msg_id       = msg_id;
        packet.header.sequence     = 0;
        packet.header.payload_len  = static_cast<uint16_t>(sizeof(T));

        packet.payload.resize(sizeof(T));

        static_assert(std::is_trivially_copyable_v<T>, "Type T must be trivially copyable for binary serialization");
        ByteUtilities::serialize_type(packet.payload.data(), custom_struct);

        return packet;
    }
};
