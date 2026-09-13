#pragma once
#include "packet.h"

class RoCommLinkParser {
 private:
    // Enumeration using fully-spelled names for state indicators
    enum class State {
        WaitStartOfTransmission,
        WaitSystemIdentifier,
        WaitMessageIdentifierHighByte,
        WaitMessageIdentifierLowByte,
        WaitPayloadLength,
        WaitPayloadData,
        WaitChecksumVerification
    };

    State   _state = State::WaitStartOfTransmission;
    Packet  _current_packet;
    uint8_t _payload_length = 0;
    uint8_t _bytes_read     = 0;

 public:
    RoCommLinkParser() = default;

    // Custom binary stream state machine parser updated with full-name state variables
    std::optional<Packet> parse_byte(uint8_t byte);
};
