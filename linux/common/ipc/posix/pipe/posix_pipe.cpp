#include "posix_pipe.h"

namespace HarisLinux {
uint64_t PosixPipe::get_current_time_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

bool PosixPipe::receive_packet(int fd, PacketHeader& header, std::vector<uint8_t>& payload) {
    if (fd == -1) return false;

    ssize_t bytes = read(fd, &header, sizeof(header));
    if (bytes <= 0) return false;

    payload.resize(header.payload_size);
    if (header.payload_size > 0) {
        ssize_t body_bytes = read(fd, payload.data(), header.payload_size);
        if (body_bytes != header.payload_size) return false;
    }
    return true;
}

}  // namespace HarisLinux