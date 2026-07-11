#pragma once
#include <cstdarg>

#include "logging/logger.h"
#include "ipc_sender.h"
#include "ipc_receiver.h"
#include "ipc_msg_dispatcher.h"

namespace HarisLinux {
/**
 * @brief Thread-safe Unified High-Performance Unix Socket Wrapper Layer.
 * @tparam Modes The template payload flag system wrapping raw configuration tags.
 * @tparam Policy Compile-time protocol selector routing static system behaviors.
 */
template <typename Modes, typename Policy>
class UniversalSocket : public IPCSenderBase<Policy>, public IPCReceiverBase<Policy> {
 private:
    // Automatically extracts the concrete context type associated with the Policy at compile-time!
    typename Policy::Context _ctx;

    const int _rx_fd;
    const int _tx_fd;

    DECLARE_LOGGER;

 public:
    // === INIT STATE (Accepts the deduced Context type based on Policy template) ===
    UniversalSocket(const typename Policy::Context& context)
        : _ctx(context),                                 //
          _rx_fd(Policy::init(_ctx, IoMode::Receiver)),  // Init receiver before transmiter
          _tx_fd(Policy::init(_ctx, IoMode::Transmiter)) {
        IPCReceiverBase<Policy>::_shared_fd = SharedFileDescription<Policy>(_rx_fd);
        IPCSenderBase<Policy>::_shared_fd   = SharedFileDescription<Policy>(_tx_fd);
        INIT_LOGGER("UnixSocket");
        logger->setLevel(LogLevel::Trace);
    }

    // === CLOSE STATE ===
    ~UniversalSocket() {
        // if (_rx_fd >= 0) ::shutdown(_rx_fd, SHUT_RDWR);
        // if (_tx_fd >= 0) ::shutdown(_tx_fd, SHUT_RDWR);

        // if (_rx_fd >= 0) ::close(_rx_fd);
        // if (_tx_fd >= 0) ::close(_tx_fd);

        // Delegate specific system-wiping cleanup hook commands back to the policy
        Policy::cleanup(_ctx);
        HARIS_LOG_DEBUG("Scope closed cleanly. Resources completely recycled.");
    }
    bool receive_packet(PacketHeader& header, std::vector<uint8_t>& payload_output) const {
        // Directly route to IPCReceiverBase core atomic reading mechanisms
        bool result = IPCReceiverBase<Policy>::receive(header, payload_output);

        if (!result) HARIS_LOG_ERROR("Got packet failed");

        /** @brief Helper support cast data type of message. */
        PacketDispatcher<DataHandlerPolicy> dispatcher{"UnixSocket"};
        // Inspect the inner content payload variations
        dispatcher.dispatch(header, payload_output);

        return result;
    }

    /**
     * @brief Unified atomic frame transmission pipeline.
     */
    template <typename T>
    bool send_packet(DataType type, const T& data, uint32_t seq) {
        bool result = IPCSenderBase<Policy>::send(type, data, seq);
        HARIS_LOG_DEBUG("------------ UnixSocket Policy Send Data ------------");
        return result;
    }
};

}  // namespace HarisLinux