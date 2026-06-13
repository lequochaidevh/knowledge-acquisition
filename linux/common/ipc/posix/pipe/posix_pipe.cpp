#include "posix_pipe.h"

namespace HarisLinux {

template <typename Modes>
bool PosixPipe<Modes>::receive_packet(int fd, PacketHeader& header, std::vector<uint8_t>& payload) {
    if (fd == -1) return false;

    int old_fd          = StreamReceiver::_fd;
    StreamReceiver::_fd = fd;

    bool result = StreamReceiver::receive(header, payload);

    StreamReceiver::_fd = old_fd;
    return result;
}

}  // namespace HarisLinux

// ============================================================================
// 2. EXPLICIT INSTANTIATION (Put this at the very bottom, outside namespaces)
// ============================================================================
template class HarisLinux::PosixPipe<HarisLinux::Ipc::Client>;
template class HarisLinux::PosixPipe<HarisLinux::Ipc::Server>;
