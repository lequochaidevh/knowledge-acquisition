#include "service/rocommlink.h"

void RoCommLink::process_raw_bytes(std::vector<uint8_t> bytes) {
    for (uint8_t byte : bytes) {
        std::lock_guard<std::shared_mutex> lock(_parsers_mutex);
        // Defaulting stream handling to slot 0 for unstructured point-to-point I/O
        if (_parsers.find(0) == _parsers.end()) {
            _parsers[0] = std::make_unique<RoCommLinkParser>();
        }

        if (auto packet_opt = _parsers[0]->parse_byte(byte); packet_opt.has_value()) {
            _dispatcher->dispatch(packet_opt.value());
        }
    }
}

RoCommLink::RoCommLink(std::unique_ptr<IOInterface> transport, size_t thread_count)
    : _transport(std::move(transport)),
      _worker_pool(std::make_unique<TaskQueue>(thread_count)),
      _dispatcher(std::make_unique<RoCommLinkDispatcher>()) {}

RoCommLink::~RoCommLink() { stop(); }

bool RoCommLink::start(const std::string& target_ip, uint16_t port) {
    _transport->register_read_callback([this](const uint8_t* data, size_t size) {
        std::vector<uint8_t> buffer(data, data + size);
        _worker_pool->push([this, buf = std::move(buffer)]() mutable { this->process_raw_bytes(std::move(buf)); });
    });

    return _transport->connect(target_ip, port);
}

void RoCommLink::stop() {
    if (_transport) _transport->disconnect();
    if (_worker_pool) _worker_pool->shutdown();
}

RoCommLinkDispatcher& RoCommLink::dispatcher() { return *_dispatcher; }

bool RoCommLink::send_packet(const Packet& packet) {
    std::vector<uint8_t> frame;
    frame.reserve(6 + packet.payload.size());

    frame.push_back(0xAA);  // STX
    frame.push_back(packet.system_id);
    frame.push_back(static_cast<uint8_t>((packet.msg_id >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(packet.msg_id & 0xFF));
    frame.push_back(static_cast<uint8_t>(packet.payload.size()));
    frame.insert(frame.end(), packet.payload.begin(), packet.payload.end());
    frame.push_back(static_cast<uint8_t>(packet.checksum & 0xFF));

    return _transport->send(frame.data(), frame.size());
}
