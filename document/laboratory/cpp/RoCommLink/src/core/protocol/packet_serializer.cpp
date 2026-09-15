#include "protocol/packet_serializer.h"

std::vector<uint8_t> PacketSerializer::serialize(const Packet& packet) {
    std::vector<uint8_t> frame;

    constexpr size_t HEADER_WIRE_SIZE = 7;
    frame.reserve(HEADER_WIRE_SIZE + packet.payload.size() + sizeof(uint16_t));

    frame.push_back(packet.header.magic);

    // 2. Write tracking parameters
    frame.push_back(packet.header.system_id);

    // 3. Deconstruct 16-bit message identifier into high and low byte sequences
    frame.push_back(static_cast<uint8_t>((packet.header.msg_id >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(packet.header.msg_id & 0xFF));

    // 4. Write variable payload bound size
    uint16_t actual_payload_len = static_cast<uint16_t>(packet.payload.size());
    frame.push_back(static_cast<uint8_t>((actual_payload_len >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(actual_payload_len & 0xFF));

    // 5. Append data segment block
    frame.insert(frame.end(), packet.payload.begin(), packet.payload.end());

    // 6. Compute real-time frame checksum including payload elements
    uint16_t computed_checksum = ByteUtilities::calculate_checksum(frame.data(), frame.size());

    // 7. Append checksum indicator byte (extract low byte matching parser rules)
    frame.push_back(static_cast<uint8_t>((computed_checksum >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(computed_checksum & 0xFF));

    return frame;
}