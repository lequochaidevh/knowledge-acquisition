#pragma once

#include "transport/io_interface.h"
#include "common/task_queue.h"
#include "protocol/rocommlink_parser.h"
#include "rocommlink_dispatcher.h"
#include "service/command_tracker.h"
#include "protocol/packet_serializer.h"

class RoCommLink {
 private:
    using ForwardByteCallback = std::function<void(uint8_t byte)>;

    // A centralized collection of all operational communication pipelines
    std::unordered_map<uint8_t, std::unique_ptr<IOInterface>> _transports;

    // Fast compound key layout linking target coordinates straight to an output interface ID
    // Key formula: (system_id << 8) | component_id
    std::unordered_map<uint16_t, uint8_t> _routing_table;
    // Fallback broadcast network interface channel used if a target address isn't registered
    static constexpr uint8_t DEFAULT_BROADCAST_INTERFACE = 0;  // Core routing execution method requested
    uint8_t                  determine_routing_target(uint8_t system_id, uint8_t component_id) const;

    std::unique_ptr<IOInterface>          _transport;  // TODO -> Make _transports list
    std::unique_ptr<TaskQueue>            _worker_pool;
    std::unique_ptr<RoCommLinkDispatcher> _dispatcher;

    // Per-system parser state maps to ensure data safety if streams interleave
    std::unordered_map<uint8_t, std::unique_ptr<RoCommLinkParser>> _parsers;
    mutable std::shared_mutex                                      _parsers_mutex;

    std::unique_ptr<CommandTracker> _command_tracker;

    // Declare the missing asynchronous thread control variables here
    std::atomic<bool> _is_running{false};
    std::thread       _timeout_thread;

    void process_raw_bytes(std::string_view bytes);

    ForwardByteCallback _forward_handler_cb = nullptr;

 public:
    // Dynamically register a new physical interface link to the active runtime system
    void add_transport(uint8_t interface_id, std::unique_ptr<IOInterface> transport);
    void add_routing_rule(uint8_t target_sys_id, uint8_t target_comp_id, uint8_t out_interface_id);

    void register_forward_handler(ForwardByteCallback callback) { _forward_handler_cb = std::move(callback); }

    RoCommLink(std::unique_ptr<IOInterface> transport, size_t thread_count = 2);

    ~RoCommLink();

    // bool start(const std::string& target_ip, uint16_t port);
    bool start(const std::string& target, uint16_t local_port, uint16_t remote_port);

    void stop();

    RoCommLinkDispatcher& dispatcher();

    // Outbound serialization helper interface for custom packets
    bool send_packet(const Packet& packet);

    void send_command_blocking(const Packet& cmd_pkt);
};