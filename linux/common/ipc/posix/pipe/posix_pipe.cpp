#include "posix_pipe.h"

namespace HarisLinux {

bool PosixPipe::receive_packet(int fd, PacketHeader& header, std::vector<uint8_t>& payload) {
    if (fd == -1) return false;

    int old_fd          = StreamReceiver::_fd;
    StreamReceiver::_fd = fd;

    bool result = StreamReceiver::receive(header, payload);

    StreamReceiver::_fd = old_fd;
    return result;
}

}  // namespace HarisLinux
