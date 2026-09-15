#include "protocol/rocommlink_parser.h"

std::optional<Packet> RoCommLinkParser::parse_byte(uint8_t byte) {
    switch (_state) {
        case State::WaitStartOfTransmission:
            if (byte == 0xAA) {
                _current_packet = Packet();
                _state          = State::WaitSystemIdentifier;
            }
            break;

        case State::WaitSystemIdentifier:
            _current_packet.header.system_id = byte;
            _state                           = State::WaitMessageIdentifierHighByte;
            break;

        case State::WaitMessageIdentifierHighByte:
            _current_packet.header.msg_id = static_cast<uint16_t>(byte << 8);
            _state                        = State::WaitMessageIdentifierLowByte;
            break;

        case State::WaitMessageIdentifierLowByte:
            _current_packet.header.msg_id |= byte;
            _state = State::WaitPayloadLength;
            break;

        case State::WaitPayloadLength:
            _payload_length = byte;
            if (_payload_length > 0) {
                _current_packet.payload.reserve(_payload_length);
                _bytes_read = 0;
                _state      = State::WaitPayloadData;
            } else {
                _state = State::WaitChecksumVerification;
            }
            break;

        case State::WaitPayloadData:
            _current_packet.payload.push_back(byte);
            _bytes_read++;
            if (_bytes_read >= _payload_length) {
                _state = State::WaitChecksumVerification;
            }
            break;

        case State::WaitChecksumVerification:
            // For simplicity, treat this single byte as a dummy checksum verification check
            _current_packet.checksum = byte;
            _state                   = State::WaitStartOfTransmission;  // Reset state machine
            return std::move(_current_packet);                          // Yield complete custom packet
    }

    return std::nullopt;
}
