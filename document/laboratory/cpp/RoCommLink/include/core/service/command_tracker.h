#include "std17pch.h"
#include "protocol/packet.h"

class CommandTracker {
 public:
    struct Transaction {
        uint16_t                              seq;
        Packet                                packet;
        std::promise<CommandResult>           promise;
        std::chrono::steady_clock::time_point last_sent;
        uint8_t                               retries = 0;
    };

    explicit CommandTracker() = default;
    ~CommandTracker()         = default;

    CommandTracker(const CommandTracker&) = delete;
    CommandTracker& operator=(const CommandTracker&) = delete;

    std::future<CommandResult> track(uint16_t seq, Packet pkt) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto [it, inserted] = _transactions.emplace(
            seq, Transaction{seq, std::move(pkt), std::promise<CommandResult>(), std::chrono::steady_clock::now(), 0});
        return it->second.promise.get_future();
    }

    void resolve(uint16_t seq, CommandResult result) {
        std::lock_guard<std::mutex> lock(_mutex);
        if (auto it = _transactions.find(seq); it != _transactions.end()) {
            it->second.promise.set_value(result);
            _transactions.erase(it);
        }
    }

    void check_timeouts(const std::function<void(const Packet&)>& retry_callback) {
        std::lock_guard<std::mutex> lock(_mutex);
        auto                        now              = std::chrono::steady_clock::now();
        const auto                  timeout_duration = std::chrono::milliseconds(500);
        const uint8_t               max_retries      = 3;

        for (auto it = _transactions.begin(); it != _transactions.end();) {
            if (now - it->second.last_sent > timeout_duration) {
                if (it->second.retries < max_retries) {
                    it->second.retries++;
                    it->second.last_sent = now;
                    retry_callback(it->second.packet);
                    ++it;
                    std::cout << "[_command_tracker->check_timeouts]\n";
                } else {
                    it->second.promise.set_value(CommandResult::FAILED);
                    it = _transactions.erase(it);
                }
            } else {
                ++it;
            }
        }
    }

 private:
    std::mutex                                _mutex;
    std::unordered_map<uint16_t, Transaction> _transactions;
};