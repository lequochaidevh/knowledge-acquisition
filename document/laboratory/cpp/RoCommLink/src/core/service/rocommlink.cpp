#include "service/rocommlink.h"

RoCommLink::RoCommLink(std::unique_ptr<IOInterface> transport, size_t thread_count)
    : _transport(std::move(transport)),
      _worker_pool(std::make_unique<TaskQueue>(thread_count)),
      _dispatcher(std::make_unique<RoCommLinkDispatcher>()),
      _command_tracker(std::make_unique<CommandTracker>()) {}

RoCommLink::~RoCommLink() { stop(); }

bool RoCommLink::start(const std::string& target_ip, uint16_t port) {
    if (!_transport) return false;

    // Register modern C++17 string_view zero-allocation read callback
    _transport->register_read_callback([this](std::string_view data) {
        if (data.empty()) return;

        // Safely capture and move a heap-allocated string allocation into worker pool
        _worker_pool->push([this, buf = std::string(data)]() mutable { this->process_raw_bytes(std::move(buf)); });
    });

    if (!_transport->connect(target_ip, port)) {
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

void RoCommLink::send_command_blocking(Packet& cmd_pkt) {
    uint16_t seq = cmd_pkt.header.sequence;

    std::future<CommandResult> ack_future = _command_tracker->track(seq, cmd_pkt);

    if (!send_packet(cmd_pkt)) {
        _command_tracker->resolve(seq, CommandResult::FAILED);
        return;
    }

    if (ack_future.wait_for(std::chrono::seconds(2)) == std::future_status::ready) {
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

    RoCommLinkParser* parser_ptr       = nullptr;
    uint8_t           static_system_id = 0;

    {
        std::shared_lock<std::shared_mutex> read_lock(_parsers_mutex);
        auto                                it = _parsers.find(static_system_id);
        if (it != _parsers.end()) {
            parser_ptr = it->second.get();
        }
    }

    if (!parser_ptr) {
        std::unique_lock<std::shared_mutex> write_lock(_parsers_mutex);
        if (_parsers.find(static_system_id) == _parsers.end()) {
            _parsers[static_system_id] = std::make_unique<RoCommLinkParser>();
        }
        parser_ptr = _parsers[static_system_id].get();
    }

    constexpr uint16_t MSG_ID_COMMAND_ACK  = 0x00FF;
    constexpr uint16_t MSG_ID_USER_COMMAND = 0x0001;

    for (char byte : bytes) {
        uint8_t raw_byte = static_cast<uint8_t>(byte);

        if (auto packet_opt = parser_ptr->parse_byte(raw_byte); packet_opt.has_value()) {
            Packet packet = std::move(packet_opt.value());

            if (packet.header.msg_id == MSG_ID_COMMAND_ACK) {
                if (packet.payload.size() >= sizeof(CommandAckPayload)) {
                    CommandAckPayload ack_data;
                    std::memcpy(&ack_data, packet.payload.data(), sizeof(CommandAckPayload));
                    _command_tracker->resolve(ack_data.target_sequence, static_cast<CommandResult>(ack_data.result));
                }
                continue;
            }

            if (packet.header.msg_id == MSG_ID_USER_COMMAND) {
                bool          success = _dispatcher->dispatch(packet);
                CommandResult result  = success ? CommandResult::ACCEPTED : CommandResult::DENIED;

                Packet ack_pkt{};
                ack_pkt.header.magic        = 0x5A;
                ack_pkt.header.version      = packet.header.version;
                ack_pkt.header.system_id    = packet.header.system_id;
                ack_pkt.header.component_id = packet.header.component_id;
                ack_pkt.header.msg_id       = MSG_ID_COMMAND_ACK;
                ack_pkt.header.sequence     = packet.header.sequence;
                ack_pkt.header.payload_len  = sizeof(CommandAckPayload);

                CommandAckPayload ack_payload{};
                ack_payload.command_msg_id  = packet.header.msg_id;
                ack_payload.target_sequence = packet.header.sequence;
                ack_payload.result          = static_cast<uint8_t>(result);

                ack_pkt.payload.resize(sizeof(CommandAckPayload));
                std::memcpy(ack_pkt.payload.data(), &ack_payload, sizeof(CommandAckPayload));

                send_packet(ack_pkt);
            } else {
                _dispatcher->dispatch(std::move(packet));
            }
        }
    }
}