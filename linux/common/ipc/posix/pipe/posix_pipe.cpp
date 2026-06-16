#include "posix_pipe.h"

namespace HarisLinux {

template <typename Modes>
bool PosixPipe<Modes>::receive_packet(int read_fd, PacketHeader& header, std::vector<uint8_t>& payload) {
    if (read_fd == -1) return false;
    int old_fd          = StreamReceiver::_fd;
    StreamReceiver::_fd = read_fd;

    bool result = StreamReceiver::receive(header, payload);

    /** @brief Helper support cast data type of message. */
    PacketDispatcher<DataHandlerPolicy> dispatcher{"PosixPipe"};
    // Inspect the inner content payload variations
    dispatcher.dispatch(header, payload);

    StreamReceiver::_fd = old_fd;
    return result;
}

}  // namespace HarisLinux

// ============================================================================
// 2. EXPLICIT INSTANTIATION (Put this at the very bottom, outside namespaces)
// ============================================================================
template class HarisLinux::PosixPipe<HarisLinux::Ipc::Client>;
template class HarisLinux::PosixPipe<HarisLinux::Ipc::Server>;
