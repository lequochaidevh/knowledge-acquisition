#include "service/rocommlink_dispatcher.h"

void RoCommLinkDispatcher::subscribe(uint16_t msg_id, MessageCallback&& cb) {
    std::unique_lock<std::shared_mutex> lock(_mutex);
    _registry[msg_id].emplace_back(std::move(cb));
}

void RoCommLinkDispatcher::dispatch(const Packet& packet) const {
    std::shared_lock<std::shared_mutex> lock(_mutex);

    auto it = _registry.find(packet.msg_id);
    if (it != _registry.end()) {
        for (const auto& callback : it->second) {
            if (callback) {
                callback(packet);
            }
        }
    }
}
