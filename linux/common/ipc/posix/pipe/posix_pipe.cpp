#include "posix_pipe.h"

namespace HarisLinux {

template <typename Modes>
bool PosixPipe<Modes>::receive_packet(PacketHeader& header, std::vector<uint8_t>& payload) {
    bool result = IPCReceiverBase<PipePolicy>::receive(header, payload);

    if (!result) {
        HARIS_LOG_WARN("Can not receive data {}", static_cast<int>(header.type));
        return result;
    }
    /** @brief Helper support cast data type of message. */
    PacketDispatcher<DataHandlerPolicy> dispatcher{"PosixPipe"};
    // Inspect the inner content payload variations
    dispatcher.dispatch(header, payload);
    return result;
}

}  // namespace HarisLinux

// ============================================================================
// 2. EXPLICIT INSTANTIATION (Put this at the very bottom, outside namespaces)
// ============================================================================
template class HarisLinux::PosixPipe<HarisLinux::Ipc::Client>;
template class HarisLinux::PosixPipe<HarisLinux::Ipc::Server>;
