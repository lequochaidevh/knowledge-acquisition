#pragma once
#include <optional>
#include <cstdint>
#include <vector>

#include "Packet.h"

class ComlinkParser {
 private:
    enum class State { WAIT_STX, WAIT_SYS_ID, WAIT_MSG_ID_H, WAIT_MSG_ID_L, WAIT_LENGTH, WAIT_PAYLOAD, WAIT_CHECKSUM };

    State   _state = State::WAIT_STX;
    Packet  _current_packet;
    uint8_t _payload_length = 0;
    uint8_t _bytes_read     = 0;

 public:
    ComlinkParser() = default;

    // Custom binary stream state machine parser
    std::optional<Packet> parse_byte(uint8_t byte) {
        switch (_state) {
            case State::WAIT_STX:
                if (byte == 0xAA) {  // Start transmission delimiter
                    _current_packet = Packet();
                    _state          = State::WAIT_SYS_ID;
                }
                break;

            case State::WAIT_SYS_ID:
                _current_packet.system_id = byte;
                _state                    = State::WAIT_MSG_ID_H;
                break;

            case State::WAIT_MSG_ID_H:
                _current_packet.msg_id = static_cast<uint16_t>(byte << 8);
                _state                 = State::WAIT_MSG_ID_L;
                break;

            case State::WAIT_MSG_ID_L:
                _current_packet.msg_id |= byte;
                _state = State::WAIT_LENGTH;
                break;

            case State::WAIT_LENGTH:
                _payload_length = byte;
                if (_payload_length > 0) {
                    _current_packet.payload.reserve(_payload_length);
                    _bytes_read = 0;
                    _state      = State::WAIT_PAYLOAD;
                } else {
                    _state = State::WAIT_CHECKSUM;
                }
                break;

            case State::WAIT_PAYLOAD:
                _current_packet.payload.push_back(byte);
                _bytes_read++;
                if (_bytes_read >= _payload_length) {
                    _state = State::WAIT_CHECKSUM;
                }
                break;

            case State::WAIT_CHECKSUM:
                // For simplicity, treat this single byte as a dummy checksum verification check
                _current_packet.checksum = byte;
                _state                   = State::WAIT_STX;  // Reset state machine
                return std::move(_current_packet);           // Yield complete custom packet
        }
        return std::nullopt;
    }
};
