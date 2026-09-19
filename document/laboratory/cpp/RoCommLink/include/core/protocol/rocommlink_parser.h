#pragma once
#include "packet.h"

// Operational results returned after evaluating a single byte
enum class ParseStatus : uint8_t {
    Processing,        // Normal local byte accumulation
    Complete,          // A full valid packet has been completely assembled
    StreamForwardByte  // This byte belongs to a foreign packet and must be forwarded right now!
};

struct ParseResult {
    ParseStatus           status = ParseStatus::Processing;
    std::optional<Packet> packet = std::nullopt;
};

class RoCommLinkParser {
 public:
    // Enumeration using fully-spelled names for state indicators
    enum class State {
        WaitStartOfTransmission,
        WaitSystemIdentifier,
        WaitMessageIdentifierHighByte,
        WaitMessageIdentifierLowByte,
        WaitPayloadLength,
        WaitPayloadData,
        WaitPayloadSkip,
        ForwardActive,  // ➔ THE SINGLE UNIFIED BYPASS STATE
        WaitChecksumVerification
    };
    // Highly flexible operational behaviors based on physical network requirements
    enum class RoutingAction : uint8_t {
        ConsumeLocal,      // Internal communication: Full payload parsing + CRC validation
        ForwardStreaming,  // Foreign network line: Byte-by-byte ultra-low latency redirection (No CRC check)
        ForwardVerified    // RF/Long-range line: Zero-copy buffering, transmits only upon successful CRC validation
    };

    // Typedef for structural boundary decoupling
    // Callback executed the moment magic, version, system_id, and component_id are resolved
    using EarlyFilterCallback = std::function<bool(Packet& packet, State& state)>;

    // Registers the functional controller mapping structural header rules to execution behaviors
    void register_early_filter(EarlyFilterCallback&& callback);

    RoCommLinkParser() = default;

    // Custom binary stream state machine parser updated with full-name state variables
    ParseResult parse_byte(uint8_t byte);

 private:
    State   _state = State::WaitStartOfTransmission;
    Packet  _current_packet;
    uint8_t _payload_length = 0;
    uint8_t _bytes_read     = 0;

    std::vector<uint8_t> _raw_frame_buffer;

    // Internal behavior modifiers dictated by runtime business policies
    EarlyFilterCallback _early_filter_cb = nullptr;
    // Centrally isolated macro/helper method to keep state switch cases tidy
    bool execute_early_filter() {
        if (_early_filter_cb && _early_filter_cb(_current_packet, _state)) {
            std::cout << "[Parser] Active early filter callback applied.\n";
            return true;
        }
        return false;
    }
};
