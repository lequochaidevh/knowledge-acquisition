#include "protocol/packet_serializer.h"

std::vector<uint8_t> PacketSerializer::serialize(const Packet& packet) {
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
