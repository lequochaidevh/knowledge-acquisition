#pragma once
#include <unordered_map>
#include <vector>
#include <functional>
#include <shared_mutex>
#include "Packet.h"

class ComlinkDispatcher {
 public:
    using MessageCallback = std::function<void(const Packet&)>;

 private:
    std::unordered_map<uint16_t, std::vector<MessageCallback>> _registry;
    mutable std::shared_mutex                                  _mutex;

 public:
    ComlinkDispatcher() = default;

    void subscribe(uint16_t msg_id, MessageCallback&& cb) {
        std::unique_lock<std::shared_mutex> lock(_mutex);
        _registry[msg_id].emplace_back(std::move(cb));
    }

    void dispatch(const Packet& packet) const {
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
};