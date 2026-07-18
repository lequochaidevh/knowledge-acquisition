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

    if (header.has_check_sum) {
        HARIS_LOG_INFO("Check sum data from sender: {}", static_cast<uint32_t>(header.check_sum_calculated));
        const uint8_t* payload_bytes = payload.data();

        uint32_t runtime_checksum =  //
            HarisLinux::ConstexprUtil::calculate_crc32(payload_bytes, header.payload_size);
        if (runtime_checksum == header.check_sum_calculated) {
            HARIS_LOG_DEBUG("Data integrity verified! Checksum matches.");
        } else {
            HARIS_LOG_ERROR("Data corrupted! Local: {} vs Sender: {}",  //
                            static_cast<uint32_t>(runtime_checksum),    //
                            static_cast<uint32_t>(header.check_sum_calculated));
        }
    }

    return result;
}

}  // namespace HarisLinux

// ============================================================================
// 2. EXPLICIT INSTANTIATION (Put this at the very bottom, outside namespaces)
// ============================================================================
template class HarisLinux::PosixPipe<HarisLinux::Ipc::Client>;
template class HarisLinux::PosixPipe<HarisLinux::Ipc::Server>;
