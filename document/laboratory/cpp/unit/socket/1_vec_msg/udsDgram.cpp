#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <atomic>
#include <thread>
#include <algorithm>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/un.h>   // Unix Domain Sockets
#include <sys/uio.h>  // Vector I/O
#include <cstdint>

enum class AddrFamily { IPv4, UNIX };

struct PacketHeader {
    uint32_t data_type;
    uint32_t payload_size;
    uint32_t seq;
};

template <AddrFamily Family>
class UdpFlexibleNode {
 private:
    int         _tx_fd = -1;
    int         _rx_fd = -1;
    std::string _local_endpoint;   // Can be "IP" or "/path/to/sock"
    std::string _target_endpoint;  // Can be "IP" or "/path/to/sock"
    uint16_t    _local_port  = 0;
    uint16_t    _target_port = 0;

    std::atomic<bool> _is_running{true};
    std::thread       _receive_thread;

    // Background thread loop handling scatter-gather network I/O
    void receive_loop() {
        std::cout << "[Node] Background receiver thread started.\n";

        while (_is_running) {
            PacketHeader      header;
            std::vector<char> payload_buffer(1024);

            struct iovec iov[2];

            iov[0].iov_base = &header;
            iov[0].iov_len  = sizeof(PacketHeader);

            iov[1].iov_base = payload_buffer.data();
            iov[1].iov_len  = payload_buffer.size();

            struct sockaddr_storage sender_addr;
            //  std::memset(&sender_addr, 0, sizeof(sender_addr));

            struct msghdr msg;
            //  std::memset(&msg, 0, sizeof(msg));
            msg.msg_name    = &sender_addr;
            msg.msg_namelen = sizeof(sender_addr);

            msg.msg_iov    = iov;
            msg.msg_iovlen = 2;

            ssize_t received = recvmsg(_rx_fd, &msg, 0);

            if (received < 0) {
                if (!_is_running) break;
                std::cerr << "[Receiver Error] recvmsg failed.\n";
                break;
            }

            if (received >= static_cast<ssize_t>(sizeof(PacketHeader))) {
                uint32_t safe_size =
                    std::min(header.payload_size, static_cast<uint32_t>(received - sizeof(PacketHeader)));
                std::string content(payload_buffer.data(), safe_size);

                std::cout << "[Node Receiver] Decoded Header -> Type: " << header.data_type << " | Seq: " << header.seq
                          << " | Size: " << header.payload_size << "\n"
                          << "[Node Receiver] Payload content: " << content << "\n";
            }
        }
    }

 public:
    // === INIT STATE (Evaluated entirely at compile-time via if constexpr) ===
    UdpFlexibleNode(const std::string& local, uint16_t l_port, const std::string& target, uint16_t t_port)
        : _local_endpoint(local), _target_endpoint(target), _local_port(l_port), _target_port(t_port) {
        // Compile-time evaluation of the underlying domain family
        if constexpr (Family == AddrFamily::IPv4) {
            _rx_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            _tx_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

            sockaddr_in local_addr;
            //  std::memset(&local_addr, 0, sizeof(local_addr));
            local_addr.sin_family = AF_INET;
            local_addr.sin_port   = htons(_local_port);
            inet_pton(AF_INET, _local_endpoint.c_str(), &local_addr.sin_addr);
            bind(_rx_fd, (sockaddr*)&local_addr, sizeof(local_addr));

            sockaddr_in target_addr;
            //  std::memset(&target_addr, 0, sizeof(target_addr));
            target_addr.sin_family = AF_INET;
            target_addr.sin_port   = htons(_target_port);
            inet_pton(AF_INET, _target_endpoint.c_str(), &target_addr.sin_addr);
            connect(_tx_fd, (sockaddr*)&target_addr, sizeof(target_addr));

            std::cout << "[Init] Configured IPv4 Stack on " << _local_endpoint << ":" << _local_port << "\n";

        } else if constexpr (Family == AddrFamily::UNIX) {
            _rx_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
            _tx_fd = socket(AF_UNIX, SOCK_DGRAM, 0);

            struct sockaddr_un local_addr;
            //  std::memset(&local_addr, 0, sizeof(local_addr));
            local_addr.sun_family = AF_UNIX;
            std::strncpy(local_addr.sun_path, _local_endpoint.c_str(), sizeof(local_addr.sun_path) - 1);

            ::unlink(_local_endpoint.c_str());  // Erase stale socket files on local disk
            bind(_rx_fd, (struct sockaddr*)&local_addr, sizeof(local_addr));

            struct sockaddr_un target_addr;
            //  std::memset(&target_addr, 0, sizeof(target_addr));
            target_addr.sun_family = AF_UNIX;
            std::strncpy(target_addr.sun_path, _target_endpoint.c_str(), sizeof(target_addr.sun_path) - 1);
            connect(_tx_fd, (struct sockaddr*)&target_addr, sizeof(target_addr));

            std::cout << "[Init] Configured Local Unix Domain Socket on: " << _local_endpoint << "\n";
        }

        // Boot async reception thread tasks immediately
        _receive_thread = std::thread(&UdpFlexibleNode::receive_loop, this);
    }

    // === CLOSE STATE ===
    ~UdpFlexibleNode() {
        _is_running = false;

        if (_rx_fd >= 0) {
            shutdown(_rx_fd, SHUT_RDWR);
            close(_rx_fd);

            // Clean filesystem footprints at compile-time if operating under UNIX structures
            if constexpr (Family == AddrFamily::UNIX) {
                ::unlink(_local_endpoint.c_str());
                std::cout << "[Cleanup] Erased file socket node: " << _local_endpoint << "\n";
            }
        }

        if (_tx_fd >= 0) {
            shutdown(_tx_fd, SHUT_RDWR);
            close(_tx_fd);
        }

        if (_receive_thread.joinable()) {
            _receive_thread.join();
        }
        std::cout << "[Cleanup] Node resources completely reclaimed.\n";
    }

    // Unified transmission method running at native hardware speeds (Zero Function Call Overhead)
    template <typename T>
    bool send_packet(uint32_t data_type, const T& container, uint32_t seq = 0) {
        if (_tx_fd < 0 || !_is_running) return false;

        PacketHeader header{data_type, static_cast<uint32_t>(container.size()), seq};
        struct iovec iov[2];

        iov[0].iov_base = &header;
        iov[0].iov_len  = sizeof(PacketHeader);

        iov[1].iov_base = const_cast<void*>(static_cast<const void*>(container.data()));
        iov[1].iov_len  = container.size() * sizeof(typename T::value_type);

        size_t total_expected = iov[0].iov_len + iov[1].iov_len;

        ssize_t sent = writev(_tx_fd, iov, 2);

        return sent == static_cast<ssize_t>(total_expected);
    }
};

// ============================================================================
// ENTRY POINT EXECUTIONS
// ============================================================================
int main() {
    std::string payload = "Data payload running inside a compile-time flexible class.";

    // ------------------------------------------------------------------------
    // CASE A: Compile and deploy purely using the IPv4 Network Stack
    // ------------------------------------------------------------------------
    std::cout << "--- Testing IPv4 Node Instantiation ---\n";
    {
        UdpFlexibleNode<AddrFamily::IPv4> ip_node("127.0.0.1", 8000, "127.0.0.1", 3000);
        UdpFlexibleNode<AddrFamily::IPv4> ip2_node("127.0.0.1", 3000, "127.0.0.1", 8000);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        ip_node.send_packet(10, payload, 123);

        std::string pl2 = "--------- payload 2";
        ip2_node.send_packet(10, pl2, 123);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }  // Out of scope: Auto triggers clean destruction path

    std::cout << "\n--- Testing Unix Domain Socket Node Instantiation ---\n";
    // ------------------------------------------------------------------------
    // CASE B: Compile and deploy purely using Unix Local File System Buffers
    // ------------------------------------------------------------------------
    {
        UdpFlexibleNode<AddrFamily::UNIX> unix_node("/tmp/flexible_B.sock", 0, "/tmp/flexible_B.sock", 0);
        std::this_thread::sleep_for(std::chrono::seconds(1));
        unix_node.send_packet(20, payload, 456);
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }  // Out of scope: Auto triggers clean destruction path and unlinks files

    return 0;
}
