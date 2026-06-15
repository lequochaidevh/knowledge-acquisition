#include "unix_socket.h"

namespace HarisLinux {

static auto logger = LogRegistry::getInstance().getLogger("UnixSocket<Modes, Transport>");

template <typename Modes, typename Transport>
uint64_t UnixSocket<Modes, Transport>::get_current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// Opens an un-bound OS file descriptor matching target address_families topologies
template <typename Modes, typename Transport>
bool UnixSocket<Modes, Transport>::initialize_socket() {
    _socket_fd = socket(_address_families, _type, 0);
    return _socket_fd != -1;
}

template <typename Modes, typename Transport>
bool UnixSocket<Modes, Transport>::configure_address(const std::string& target, int port) {
    std::memset(&_remote_addr, 0, sizeof(_remote_addr));

    if (_address_families == AF_UNIX) {
        auto* addr       = reinterpret_cast<sockaddr_un*>(&_remote_addr);
        addr->sun_family = AF_UNIX;
        std::strncpy(addr->sun_path, target.c_str(), sizeof(addr->sun_path) - 1);
        _remote_addr_len = sizeof(sockaddr_un);
        return true;
    } else if (_address_families == AF_INET) {
        auto* addr       = reinterpret_cast<sockaddr_in*>(&_remote_addr);
        addr->sin_family = AF_INET;
        addr->sin_port   = htons(port);
        if (inet_pton(AF_INET, target.c_str(), &addr->sin_addr) <= 0) return false;
        _remote_addr_len = sizeof(sockaddr_in);
        return true;
    } else if (_address_families == AF_INET6) {
        auto* addr        = reinterpret_cast<sockaddr_in6*>(&_remote_addr);
        addr->sin6_family = AF_INET6;
        addr->sin6_port   = htons(port);
        if (inet_pton(AF_INET6, target.c_str(), &addr->sin6_addr) <= 0) return false;
        _remote_addr_len = sizeof(sockaddr_in6);
        return true;
    }
    return false;
}

template <typename Modes, typename Transport>
bool UnixSocket<Modes, Transport>::receive_packet(int source_fd, PacketHeader& out_header,
                                                  std::vector<uint8_t>& out_payload) {
    if (source_fd == -1) return false;

    // Hot-swap the base class FD using compile-time selection
    int old_fd        = ReceiverBase::_fd;
    ReceiverBase::_fd = source_fd;

    // Evaluated entirely at compile-time
    bool result = ReceiverBase::receive(out_header, out_payload);

    if constexpr (std::is_same_v<Transport, Ipc::StreamTag>) {
        HARIS_LOG_DEBUG("------------ Socket Stream Send Data ------------");
    } else {
        HARIS_LOG_DEBUG("------------ Socket Dgram Send Data ------------");
    }

    ReceiverBase::_fd = old_fd;

    /** @brief Helper support cast data type of message. */
    PacketDispatcher<DataHandlerPolicy> dispatcher{"UnixSocket"};
    // Inspect the inner content payload variations
    dispatcher.dispatch(out_header, out_payload);

    return result;
}

// =========================================================================
// EXPLICIT TEMPLATE INSTANTIATIONS FOR HARIS_LINUX LINKER PIPELINES
// =========================================================================

// Generate machine code for the Stream variant (TCP Server)
template class HarisLinux::UnixSocket<HarisLinux::Ipc::Server, HarisLinux::Ipc::StreamTag>;

// Generate machine code for the Datagram variant (UDP Client)
template class HarisLinux::UnixSocket<HarisLinux::Ipc::Client, HarisLinux::Ipc::DgramTag>;

}  // namespace HarisLinux
