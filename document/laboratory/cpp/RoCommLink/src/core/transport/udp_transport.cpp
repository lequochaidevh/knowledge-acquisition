#include "transport/udp_transport.h"

void UdpTransport::receive_loop() {
    static constexpr size_t MAX_UDP_PACKET = 65535;
    auto                    buffer         = std::make_unique<uint8_t[]>(MAX_UDP_PACKET);
    struct sockaddr_in      src_addr {};
    socklen_t               addr_len = sizeof(src_addr);

    while (_is_running) {
        ssize_t bytes_received =
            recvfrom(_sockfd, buffer.get(), MAX_UDP_PACKET, 0, (struct sockaddr*)&src_addr, &addr_len);

        if (bytes_received < 0) {
            // Timeout in Non-blocking socket
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                continue;
            }
            // Interupt with System Signal (EINTR)
            if (errno == EINTR) {
                continue;
            }
            if (_is_running) {
                std::cerr << "[UdpTransport] Error receiving data\n";
            }
            break;
        }

        if (bytes_received > 0 && _data_callback) {
            std::string_view data_view(reinterpret_cast<const char*>(buffer.get()),
                                       static_cast<size_t>(bytes_received));
            _data_callback(data_view);
        }
    }
}

UdpTransport::~UdpTransport() { disconnect(); }

bool UdpTransport::connect(const std::string& target_ip, uint16_t local_port, uint16_t remote_port) {
    _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (_sockfd < 0) {
        std::cerr << "[UdpTransport] Failed to create socket\n";
        return false;
    }

    int opt = 1;
    setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in local_addr {};
    std::memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family      = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port        = htons(local_port);

    if (bind(_sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        std::cerr << "[UdpTransport] Bind failed on local port: " << local_port << "\n";
        close(_sockfd);
        return false;
    }

    std::memset(&_target_addr, 0, sizeof(_target_addr));
    _target_addr.sin_family = AF_INET;
    _target_addr.sin_port   = htons(remote_port);

    if (inet_pton(AF_INET, target_ip.c_str(), &_target_addr.sin_addr) <= 0) {
        std::cerr << "[UdpTransport] Invalid IP address\n";
        close(_sockfd);
        return false;
    }

    struct timeval tv;
    tv.tv_sec  = 0;
    tv.tv_usec = 100000;
    setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    _is_running  = true;
    _recv_thread = std::thread(&UdpTransport::receive_loop, this);
    std::cout << "[UdpTransport] Connected. Listening on port " << local_port << " -> Targeting remote port "
              << remote_port << "\n";
    return true;
}

void UdpTransport::disconnect() {
    _is_running = false;
    if (_sockfd >= 0) {
        // Shutdown socket to unblock recvfrom immediately
        shutdown(_sockfd, SHUT_RDWR);
        close(_sockfd);
        _sockfd = -1;
    }
    if (_recv_thread.joinable()) {
        _recv_thread.join();
    }
    std::cout << "[UdpTransport] Disconnected cleanly\n";
}

bool UdpTransport::send(std::string_view data) {
    if (_sockfd < 0 || data.empty()) return false;
    std::cout << "[UdpTransport] C1\n";
    ssize_t bytes_sent =
        sendto(_sockfd, data.data(), data.size(), 0, (struct sockaddr*)&_target_addr, sizeof(_target_addr));

    std::cout << "[UdpTransport] C2\n";
    return bytes_sent == static_cast<ssize_t>(data.size());
}
