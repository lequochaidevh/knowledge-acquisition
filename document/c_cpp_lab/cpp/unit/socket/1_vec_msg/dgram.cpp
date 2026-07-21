#include <iostream>
#include <thread>
#include <atomic>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdint>
#include <sys/uio.h>  // Required for iovec, writev, and recvmsg
#include <vector>

std::atomic<bool> isRunning(true);
int               recvSocket = -1;
int               sendSocket = -1;

struct PacketHeader {
    uint32_t data_type;     // Application-defined identifier
    uint32_t payload_size;  // Size of the dynamic data following header
    uint32_t seq;           // Sequence counter for tracking order
};

class UdpNode {
 private:
    int _tx_fd;  // Dedicated File Descriptor for Transmitting
    int _rx_fd;  // Dedicated File Descriptor for Receiving

    sockaddr_in       _local_addr;
    sockaddr_in       _target_addr;
    std::atomic<bool> _is_running;
    std::thread       _receive_thread;

    // Internal background thread loop for continuous data reception
    void receive_loop() {
        std::cout << "[Node] Background receiver thread started using RX FD: " << _rx_fd << "\n";

        while (_is_running) {
            PacketHeader      header;
            std::vector<char> payload_buffer(1024);  // Staging buffer for payload data

            // Define the 2-segment vector array to receive data
            struct iovec iov[2];
            // Segment 1: Network bytes go straight into the struct fields
            iov[0].iov_base = &header;
            iov[0].iov_len  = sizeof(PacketHeader);

            // Segment 2: Remaining network bytes flow into the continuous dynamic array
            iov[1].iov_base = payload_buffer.data();
            iov[1].iov_len  = payload_buffer.size();

            // To use Vector I/O with an UNCONNECTED UDP socket, we must use msghdr and recvmsg()
            // (Vector I/O with socket need msghdr and recvmsg)
            sockaddr_in sender_addr;
            std::memset(&sender_addr, 0, sizeof(sender_addr));

            struct msghdr msg;
            std::memset(&msg, 0, sizeof(msg));
            msg.msg_name    = &sender_addr;  // Buffer to store remote sender's IP/Port
            msg.msg_namelen = sizeof(sender_addr);
            msg.msg_iov     = iov;  // Attach our iovec array pointer
            msg.msg_iovlen  = 2;    // Number of segments in iovec

            // Atomic POSIX system call to read scatter-gather network data
            ssize_t received = recvmsg(_rx_fd, &msg, 0);

            if (received < 0) {
                if (!_is_running) break;
                std::cerr << "[Receiver Error] Recvmsg operation failed.\n";
                break;
            }

            // Validation check: Ensure the datagram contains at least a full header block
            if (received >= static_cast<ssize_t>(sizeof(PacketHeader))) {
                // Ensure we don't read beyond what was actually transmitted to prevent garbage strings
                uint32_t safe_size =
                    std::min(header.payload_size, static_cast<uint32_t>(received - sizeof(PacketHeader)));

                std::string content(payload_buffer.data(), safe_size);

                char sender_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip, INET_ADDRSTRLEN);

                std::cout << "\n[Node Receiver] Incoming packet from " << sender_ip << ":"
                          << ntohs(sender_addr.sin_port) << "\n"
                          << "[Node Receiver] Decoded Header -> Type: " << header.data_type << " | Seq: " << header.seq
                          << " | Size: " << header.payload_size << "\n"
                          << "[Node Receiver] Payload content: " << content << "\n";
            }
        }
        std::cout << "[Node] Background receiver thread stopped.\n";
    }

 public:
    // === INIT STATE ===
    UdpNode(const std::string& local_ip, uint16_t local_port, const std::string& target_ip, uint16_t target_port)
        : _tx_fd(-1), _rx_fd(-1), _is_running(true) {
        // -------------------------------------------------------------
        // Step 1: Initialize the dedicated RECEIVING Subsystem (RX FD)
        // -------------------------------------------------------------
        _rx_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (_rx_fd < 0) {
            throw std::runtime_error("Failed to create RX socket");
        }

        std::memset(&_local_addr, 0, sizeof(_local_addr));
        _local_addr.sin_family = AF_INET;
        _local_addr.sin_port   = htons(local_port);
        inet_pton(AF_INET, local_ip.c_str(), &_local_addr.sin_addr);

        // Bind the RX socket to the local port to actively listen for incoming data traffic
        if (bind(_rx_fd, (sockaddr*)&_local_addr, sizeof(_local_addr)) < 0) {
            close(_rx_fd);
            throw std::runtime_error("Failed to bind RX socket to local port");
        }

        // CRITICAL FIX: DO NOT call connect() on _rx_fd.
        // This allows it to receive datagrams from ephemeral ports assigned to _tx_fd.

        // -------------------------------------------------------------
        // Step 2: Initialize the dedicated TRANSMITTING Subsystem (TX FD)
        // -------------------------------------------------------------
        _tx_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (_tx_fd < 0) {
            close(_rx_fd);
            throw std::runtime_error("Failed to create TX socket");
        }

        std::memset(&_target_addr, 0, sizeof(_target_addr));
        _target_addr.sin_family = AF_INET;
        _target_addr.sin_port   = htons(target_port);
        inet_pton(AF_INET, target_ip.c_str(), &_target_addr.sin_addr);

        // Virtual-connect the TX socket to the destination endpoint to unlock writev() optimizations
        if (connect(_tx_fd, (sockaddr*)&_target_addr, sizeof(_target_addr)) < 0) {
            close(_tx_fd);
            close(_rx_fd);
            throw std::runtime_error("Failed to connect TX socket to remote target");
        }

        // -------------------------------------------------------------
        // Step 3: Spawn the asynchronous background worker execution thread
        // -------------------------------------------------------------
        _receive_thread = std::thread(&UdpNode::receive_loop, this);
    }

    // === CLOSE STATE ===
    ~UdpNode() {
        _is_running = false;

        // Forcefully interrupt hanging network reads immediately via shutdown
        if (_rx_fd >= 0) {
            shutdown(_rx_fd, SHUT_RDWR);
            close(_rx_fd);
        }

        if (_tx_fd >= 0) {
            shutdown(_tx_fd, SHUT_RDWR);
            close(_tx_fd);
        }

        if (_receive_thread.joinable()) {
            _receive_thread.join();
        }
        std::cout << "[Node] Closed cleanly. File descriptors fully reclaimed.\n";
    }

    // Standard high-performance multi-buffer transmission method
    template <typename T>
    bool send_packet(uint32_t data_type, const T& container, uint32_t seq = 0) {
        if (_tx_fd < 0 || !_is_running) return false;

        PacketHeader header{data_type, static_cast<uint32_t>(container.size()), seq};

        struct iovec iov[2];
        // IOV Segment 1: Map the transmission block header data
        iov[0].iov_base = &header;
        iov[0].iov_len  = sizeof(PacketHeader);

        // IOV Segment 2: Map the raw sequential element buffer within the data container
        iov[1].iov_base = const_cast<void*>(static_cast<const void*>(container.data()));
        iov[1].iov_len  = container.size() * sizeof(typename T::value_type);

        size_t total_expected = iov[0].iov_len + iov[1].iov_len;

        // Atomically gathers buffers and transmits them over the connected TX pipeline
        ssize_t sent = writev(_tx_fd, iov, 2);

        return sent == static_cast<ssize_t>(total_expected);
    }
};

int main() {
    try {
        // Run test scenario: App binds locally to 5000 and directs output payloads to 5000 (Loopback test)
        UdpNode node("127.0.0.1", 5000, "127.0.0.1", 1234);
        UdpNode node_2("127.0.0.1", 1234, "127.0.0.1", 5000);

        std::string payload = "Hello from a unified merged multi-threaded UDP Node!";

        std::cout << "[Main Thread] Sending packet in 1 second...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // Transmit packet through the shared file descriptor
        node.send_packet(1, payload, 777);

        node_2.send_packet(1, "2" + payload, 778);
        // Keep main execution environment alive briefly to observe the async echo capture
        std::this_thread::sleep_for(std::chrono::seconds(2));

        std::cout << "[Main Thread] Initiating shutdown procedure via Destructor...\n";
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
