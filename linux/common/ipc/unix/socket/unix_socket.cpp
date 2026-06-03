#include "unix_socket.h"

namespace HarisLinux {

uint64_t UnixSocket::get_current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

bool UnixSocket::initialize_socket() {
    _socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    return _socket_fd != -1;
}

}  // namespace HarisLinux
