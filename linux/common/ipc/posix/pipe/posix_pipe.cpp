#include "posix_pipe.h"

namespace HarisLinux {

// API send data: data acquisition
bool PosixPipe::SendPacket(int fd, DataType type, const std::vector<uint8_t>& data, uint32_t seq) {
    if (fd == -1) return false;

    PacketHeader header{type, static_cast<uint32_t>(data.size()), 0, seq};

    // Send size Header packed
    if (write(fd, &header, sizeof(PacketHeader)) != sizeof(PacketHeader)) return false;

    if (header.payload_size > 0) {
        if (write(fd, data.data(), data.size()) != static_cast<ssize_t>(data.size())) return false;
    }
    return true;
}

bool PosixPipe::ReadPacket(int fd, PacketHeader& header, std::vector<uint8_t>& data) {
    if (fd == -1) return false;

    ssize_t bytes = read(fd, &header, sizeof(PacketHeader));
    if (bytes <= 0) return false;

    data.resize(header.payload_size);
    if (header.payload_size > 0) {
        ssize_t body_bytes = read(fd, data.data(), header.payload_size);
        if (body_bytes != header.payload_size) return false;
    }
    return true;
}

}  // namespace HarisLinux