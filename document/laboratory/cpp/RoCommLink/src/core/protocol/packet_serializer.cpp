#include "protocol/packet_serializer.h"

std::vector<uint8_t> PacketSerializer::serialize(const Packet& packet) {
    std::vector<uint8_t> frame;

    // 10 bytes static header + variable data segment + 2 bytes trailing CRC
    constexpr size_t HEADER_WIRE_SIZE = 10;
    frame.reserve(HEADER_WIRE_SIZE + packet.payload.size() + sizeof(uint16_t));

    // 1. Pack wire header byte parameters
    frame.push_back(packet.header.magic);
    frame.push_back(packet.header.version);
    frame.push_back(packet.header.system_id);
    frame.push_back(packet.header.component_id);

    // 2. Deconstruct 16-bit message tracking identification token
    frame.push_back(static_cast<uint8_t>((packet.header.msg_id >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(packet.header.msg_id & 0xFF));

    // 3. Deconstruct 16-bit asynchronous transaction sequence index
    frame.push_back(static_cast<uint8_t>((packet.header.sequence >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(packet.header.sequence & 0xFF));

    // 4. Deconstruct 16-bit payload length boundary condition
    uint16_t actual_payload_len = static_cast<uint16_t>(packet.payload.size());
    frame.push_back(static_cast<uint8_t>((actual_payload_len >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(actual_payload_len & 0xFF));

    // 5. Inject continuous data block segments
    frame.insert(frame.end(), packet.payload.begin(), packet.payload.end());

    // 6. Compute mathematical frame checksum verification code
    uint16_t computed_checksum = ByteUtilities::calculate_checksum(frame.data(), frame.size());

    // 7. Append high and low bytes of computed CRC to frame boundary tail
    frame.push_back(static_cast<uint8_t>((computed_checksum >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(computed_checksum & 0xFF));

    return frame;
}