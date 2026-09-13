#pragma once
#include "protocol/packet.h"

class RoCommLinkDispatcher {
 public:
    using MessageCallback = std::function<void(const Packet&)>;

 private:
    std::unordered_map<uint16_t, std::vector<MessageCallback>> _registry;
    mutable std::shared_mutex                                  _mutex;

 public:
    RoCommLinkDispatcher() = default;

    void subscribe(uint16_t msg_id, MessageCallback&& cb);

    void dispatch(const Packet& packet) const;
};