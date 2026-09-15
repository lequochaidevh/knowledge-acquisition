#pragma once

#include "transport/io_interface.h"
#include "common/task_queue.h"
#include "protocol/rocommlink_parser.h"
#include "rocommlink_dispatcher.h"
#include "service/command_tracker.h"
#include "protocol/packet_serializer.h"

class RoCommLink {
 private:
    std::unique_ptr<IOInterface>          _transport;
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

 public:
    RoCommLink(std::unique_ptr<IOInterface> transport, size_t thread_count = 2);

    ~RoCommLink();

    bool start(const std::string& target_ip, uint16_t port);

    void stop();

    RoCommLinkDispatcher& dispatcher();

    // Outbound serialization helper interface for custom packets
    bool send_packet(const Packet& packet);

    void send_command_blocking(Packet& cmd_pkt);
};