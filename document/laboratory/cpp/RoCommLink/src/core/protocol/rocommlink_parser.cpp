#include "protocol/rocommlink_parser.h"
#include "common/byte_utilities.h"

std::optional<Packet> RoCommLinkParser::parse_byte(uint8_t byte) {
    switch (_state) {
        case State::WaitStartOfTransmission:
            if (byte == 0x5A || byte == 0xAA) {
                _current_packet = Packet();
                _raw_frame_buffer.clear();  // Ensure isolated member variable buffer is completely reset
                _raw_frame_buffer.push_back(byte);
                _current_packet.header.magic = byte;
                _state                       = State::WaitSystemIdentifier;
                _bytes_read                  = 0;
            }
            break;

        case State::WaitSystemIdentifier:
            _raw_frame_buffer.push_back(byte);
            if (_bytes_read == 0) {
                _current_packet.header.version = byte;
                _bytes_read                    = 1;
            } else {
                _current_packet.header.system_id = byte;
                _state                           = State::WaitMessageIdentifierHighByte;
            }
            break;

        case State::WaitMessageIdentifierHighByte:
            _raw_frame_buffer.push_back(byte);
            _current_packet.header.component_id = byte;
            _state                              = State::WaitMessageIdentifierLowByte;
            break;

        case State::WaitMessageIdentifierLowByte:
            _raw_frame_buffer.push_back(byte);
            if (_bytes_read == 1) {
                _current_packet.header.msg_id = static_cast<uint16_t>(byte << 8);
                _bytes_read                   = 2;
            } else {
                _current_packet.header.msg_id |= byte;
                _state      = State::WaitPayloadLength;
                _bytes_read = 0;
            }
            break;

        case State::WaitPayloadLength:
            _raw_frame_buffer.push_back(byte);
            if (_bytes_read == 0) {
                _current_packet.header.sequence = static_cast<uint16_t>(byte << 8);
                _bytes_read                     = 1;
            } else if (_bytes_read == 1) {
                _current_packet.header.sequence |= byte;
                _bytes_read = 2;
            } else if (_bytes_read == 2) {
                _current_packet.header.payload_len = static_cast<uint16_t>(byte << 8);
                _bytes_read                        = 3;
            } else {
                _current_packet.header.payload_len |= byte;
                _payload_length = static_cast<uint8_t>(_current_packet.header.payload_len & 0xFF);

                if (_payload_length > 0) {
                    _current_packet.payload.resize(_payload_length);
                    _bytes_read = 0;
                    _state      = State::WaitPayloadData;
                } else {
                    _bytes_read = 0;
                    _state      = State::WaitChecksumVerification;
                }
            }
            break;

        case State::WaitPayloadData:
            _raw_frame_buffer.push_back(byte);  // Crucial: Register arriving payload bytes into checksum buffer
            _current_packet.payload[_bytes_read] = byte;
            _bytes_read++;
            if (_bytes_read >= _payload_length) {
                _bytes_read = 0;
                _state      = State::WaitChecksumVerification;
            }
            break;

        case State::WaitChecksumVerification:
            if (_bytes_read == 0) {
                _current_packet.checksum = static_cast<uint16_t>(byte << 8);
                _bytes_read              = 1;
            } else {
                _current_packet.checksum |= byte;

                uint16_t computed_checksum =
                    ByteUtilities::calculate_checksum(_raw_frame_buffer.data(), _raw_frame_buffer.size());

                _state = State::WaitStartOfTransmission;
                if (computed_checksum == _current_packet.checksum) {
                    return std::move(_current_packet);  // Yield clean packet up the dispatch line
                }
            }
            break;
    }

    return std::nullopt;
}