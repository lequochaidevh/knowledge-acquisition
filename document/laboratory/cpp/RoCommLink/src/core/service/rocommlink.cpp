#include "service/rocommlink.h"

RoCommLink::RoCommLink(std::unique_ptr<IOInterface> transport, size_t thread_count)
    : _transport(std::move(transport)),
      _worker_pool(std::make_unique<TaskQueue>(thread_count)),
      _dispatcher(std::make_unique<RoCommLinkDispatcher>()),
      _command_tracker(std::make_unique<CommandTracker>()) {}

void RoCommLink::add_transport(uint8_t interface_id, std::unique_ptr<IOInterface> transport) {
    _transports[interface_id] = std::move(transport);
}

void RoCommLink::add_routing_rule(uint8_t target_sys_id, uint8_t target_comp_id, uint8_t out_interface_id) {
    uint16_t compound_key        = (static_cast<uint16_t>(target_sys_id) << 8) | target_comp_id;
    _routing_table[compound_key] = out_interface_id;
    std::cout << "[Router] Registered route: Target Sys " << static_cast<int>(target_sys_id) << ", Comp "
              << static_cast<int>(target_comp_id) << " ➔ Interface ID " << static_cast<int>(out_interface_id) << "\n";
}

/**
 * Evaluates target coordinates against active network policy records to extract an outbound channel ID.
 * Highly scalable, clear, and executes with sub-microsecond latency.
 */
uint8_t RoCommLink::determine_routing_target(uint8_t system_id, uint8_t component_id) const {
    // Generate the unified 16-bit indexing token mapping the topological address
    uint16_t compound_key = (static_cast<uint16_t>(system_id) << 8) | component_id;

    // Perform an immediate hash look-up in the routing table database
    auto it = _routing_table.find(compound_key);
    if (it != _routing_table.end()) {
        return it->second;  // Exact directional route match found
    }

    // Advanced Fallback Rule: If specific component matching fails, check for a global system-wide channel rule
    uint16_t system_wide_key =
        (static_cast<uint16_t>(system_id) << 8) | 0x00;  // Component ID 0 implies wildcard system hub
    auto sys_it = _routing_table.find(system_wide_key);
    if (sys_it != _routing_table.end()) {
        return sys_it->second;
    }

    // No routing rules match this frame footprint; return default baseline broadcast line to prevent silent packet
    // drops
    return DEFAULT_BROADCAST_INTERFACE;
}

RoCommLink::~RoCommLink() { stop(); }

// bool RoCommLink::start(const std::string& target_ip, uint16_t port)

bool RoCommLink::start(const std::string& target, uint16_t local_port, uint16_t remote_port) {
    if (!_transport) return false;

    // Register modern C++17 string_view zero-allocation read callback
    _transport->register_read_callback([this](std::string_view data) {
        if (data.empty()) return;

        // Safely capture and move a heap-allocated string allocation into worker pool
        _worker_pool->push([this, buf = std::string(data)]() mutable { this->process_raw_bytes(std::move(buf)); });
    });

    if (!_transport->connect(target, local_port, remote_port)) {
        return false;
    }

    _is_running     = true;
    _timeout_thread = std::thread([this]() {
        while (_is_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));

            _command_tracker->check_timeouts([this](const Packet& retry_pkt) { this->send_packet(retry_pkt); });
        }
    });

    return true;
}

void RoCommLink::stop() {
    _is_running = false;

    if (_timeout_thread.joinable()) {
        _timeout_thread.join();
    }

    // Correctly call disconnect matching your IOInterface definition
    if (_transport) {
        _transport->disconnect();
    }

    if (_worker_pool) {
        _worker_pool->shutdown();
    }
}

RoCommLinkDispatcher& RoCommLink::dispatcher() { return *_dispatcher; }

bool RoCommLink::send_packet(const Packet& packet) {
    if (!_transport) return false;

    std::vector<uint8_t> serialized_vector = PacketSerializer::serialize(packet);

    // Convert std::vector<uint8_t> framework buffer cleanly into string_view zero-copy footprint
    std::string_view out_view(reinterpret_cast<const char*>(serialized_vector.data()), serialized_vector.size());
    return _transport->send(out_view);
}

void RoCommLink::send_command_blocking(const Packet& cmd_pkt) {
    uint16_t seq = cmd_pkt.header.sequence;

    std::future<CommandResult> ack_future = _command_tracker->track(seq, cmd_pkt);
    std::cout << "[RoCommLink] C1\n";

    if (!send_packet(cmd_pkt)) {
        _command_tracker->resolve(seq, CommandResult::FAILED);
        return;
    }
    std::cout << "[RoCommLink] C2\n";

    if (ack_future.wait_for(std::chrono::seconds(3)) == std::future_status::ready) {
        CommandResult result = ack_future.get();
        if (result == CommandResult::ACCEPTED) {
            std::cout << "[RoCommLink] Command acknowledged and accepted by peer.\n";
        } else {
            std::cerr << "[RoCommLink] Command rejected by peer with code: " << static_cast<int>(result) << "\n";
        }
    } else {
        std::cerr << "[RoCommLink] Transaction timeout. Remote target dropped packet connection.\n";
    }
}

void RoCommLink::process_raw_bytes(std::string_view bytes) {
    if (bytes.empty()) return;

    RoCommLinkParser* parser_ptr          = nullptr;
    uint8_t           default_stream_slot = 0;

    {
        std::shared_lock<std::shared_mutex> read_lock(_parsers_mutex);
        auto                                it = _parsers.find(default_stream_slot);
        if (it != _parsers.end()) {
            parser_ptr = it->second.get();
        }
    }

    if (!parser_ptr) {
        std::unique_lock<std::shared_mutex> write_lock(_parsers_mutex);
        if (_parsers.find(default_stream_slot) == _parsers.end()) {
            _parsers[default_stream_slot] = std::make_unique<RoCommLinkParser>();
        }
        parser_ptr = _parsers[default_stream_slot].get();
    }

    constexpr uint16_t MSG_ID_COMMAND_ACK  = 0x00FF;
    constexpr uint16_t MSG_ID_USER_COMMAND = 0x0001;

    for (char byte : bytes) {
        uint8_t raw_byte = static_cast<uint8_t>(byte);

        // Execute the state machine on the current incoming byte
        ParseResult result = parser_ptr->parse_byte(raw_byte);

        switch (result.status) {
            case ParseStatus::StreamForwardByte: {
                // ULTRA-LOW LATENCY STREAMING ROADWAY:
                // Instantly broadcast the raw incoming byte out over the external alternate interface line
                uint8_t system_id           = result.packet.value().header.system_id;
                uint8_t component_id        = result.packet.value().header.component_id;
                uint8_t target_interface_id = determine_routing_target(system_id, component_id);

                auto it = _transports.find(target_interface_id);
                if (it != _transports.end()) {
                    // Broadcast the raw byte directly out of the targeted physical pipeline interface
                    it->second->send(bytes);
                }
                break;
            }

            case ParseStatus::Complete: {
                if (!result.packet.has_value()) break;
                Packet packet = std::move(result.packet.value());

                // Route completed packets depending on their explicit tracking identification tokens
                switch (packet.header.msg_id) {
                    case MSG_ID_COMMAND_ACK: {
                        // CASE 1: Arriving verification feedback from a remote node (Handled by client side)
                        if (packet.payload.size() >= sizeof(CommandAckPayload)) {
                            CommandAckPayload ack_data;
                            std::memcpy(&ack_data, packet.payload.data(), sizeof(CommandAckPayload));
                            _command_tracker->resolve(ack_data.target_sequence,
                                                      static_cast<CommandResult>(ack_data.result));
                        }
                        break;
                    }

                    case MSG_ID_USER_COMMAND: {
                        // CASE 2: Arriving mission action execution command (Handled by server side)
                        bool          success = _dispatcher->dispatch(packet);
                        CommandResult res_val = success ? CommandResult::ACCEPTED : CommandResult::DENIED;

                        // Automated packing and structural feedback routing sequence
                        Packet ack_pkt{};
                        ack_pkt.header.magic        = 0x5A;  // Synchronized with core protocol definition
                        ack_pkt.header.system_id    = packet.header.system_id;
                        ack_pkt.header.component_id = packet.header.component_id;
                        ack_pkt.header.msg_id       = MSG_ID_COMMAND_ACK;
                        ack_pkt.header.sequence     = packet.header.sequence;  // Perfect transaction alignment
                        ack_pkt.header.payload_len  = sizeof(CommandAckPayload);

                        CommandAckPayload ack_payload{};
                        ack_payload.command_msg_id  = packet.header.msg_id;
                        ack_payload.target_sequence = packet.header.sequence;
                        ack_payload.result          = static_cast<uint8_t>(res_val);

                        ack_pkt.payload.resize(sizeof(CommandAckPayload));
                        std::memcpy(ack_pkt.payload.data(), &ack_payload, sizeof(CommandAckPayload));

                        // Instantly fire response packet out over the primary transport pipeline
                        send_packet(ack_pkt);
                        break;
                    }

                    default: {
                        // CASE 3: Unhandled custom packet variants routed to standard asynchronous task listeners
                        _dispatcher->dispatch(std::move(packet));
                        break;
                    }
                }
                break;
            }

            case ParseStatus::Processing:
            default:
                // Byte successfully injected into the state machine; keep processing the remaining stream buffer
                break;
        }
    }
}