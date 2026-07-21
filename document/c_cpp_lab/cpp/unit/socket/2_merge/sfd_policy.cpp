#include <cstdint>
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

struct PacketHeader {
    uint32_t data_type;
    uint32_t payload_size;
    uint32_t seq;
};

// ============================================================================
// PART 1: THE CONTEXTS
// ============================================================================

enum class IoMode { TRANSMIT_ONLY, RECEIVE_ONLY };

// Context: IPv4 DGRAM (IP, Port, sockaddr_in)
struct SocketDgramIPv4Context {
    std::string local_ip;
    uint16_t    local_port = 0;
    std::string target_ip;
    uint16_t    target_port = 0;

    // Runtime cache for compiled network layouts
    sockaddr_in local_sockaddr;
    sockaddr_in target_sockaddr;
};

// Context: Unix Domain DGRAM (File Paths, sockaddr_un)
struct SocketDgramPathContext {
    std::string local_path;
    std::string target_path;

    // Runtime cache for compiled local filesystem layouts
    sockaddr_un local_sockaddr;
    sockaddr_un target_sockaddr;
};

// Context: Unix Domain STREAM (File Paths, sockaddr_un)
struct SocketStreamPathContext {
    std::string local_path;
    std::string target_path;

    sockaddr_un local_sockaddr;
    sockaddr_un target_sockaddr;
};

// ============================================================================
// PART 2: THE POLICIES (STATIC COMMAND)
// ============================================================================

// --- POLICY 1: IPv4 DGRAM ---
class SocketDgramIPv4Policy {
 public:
    using Context = SocketDgramIPv4Context;

    static int init(Context& ctx, IoMode mode) {
        int fd = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (fd == -1) {
            throw std::runtime_error("IPv4 Socket creation failed");
        }

        if (mode == IoMode::RECEIVE_ONLY) {
            std::memset(&ctx.local_sockaddr, 0, sizeof(ctx.local_sockaddr));
            ctx.local_sockaddr.sin_family = AF_INET;
            ctx.local_sockaddr.sin_port   = htons(ctx.local_port);
            ::inet_pton(AF_INET, ctx.local_ip.c_str(), &ctx.local_sockaddr.sin_addr);
            ::bind(fd, (sockaddr*)&ctx.local_sockaddr, sizeof(ctx.local_sockaddr));
            std::cout << "[Policy IPv4] Generated Receiver FD: " << fd << std::endl;
        } else if (mode == IoMode::TRANSMIT_ONLY) {
            std::memset(&ctx.target_sockaddr, 0, sizeof(ctx.target_sockaddr));
            ctx.target_sockaddr.sin_family = AF_INET;
            ctx.target_sockaddr.sin_port   = htons(ctx.target_port);
            ::inet_pton(AF_INET, ctx.target_ip.c_str(), &ctx.target_sockaddr.sin_addr);
            ::connect(fd, (sockaddr*)&ctx.target_sockaddr, sizeof(ctx.target_sockaddr));
            std::cout << "[Policy IPv4] Generated Transmitter FD: " << fd << std::endl;
        }

        return fd;
    }

    static ssize_t write_vector(int tx_fd, const struct iovec* iov, int count) { return ::writev(tx_fd, iov, count); }

    static ssize_t read_vector(int rx_fd, struct iovec* iov, int count) {
        struct sockaddr_storage sender_identity;
        std::memset(&sender_identity, 0, sizeof(sender_identity));
        struct msghdr msg;
        std::memset(&msg, 0, sizeof(msg));
        msg.msg_name    = &sender_identity;
        msg.msg_namelen = sizeof(sender_identity);
        msg.msg_iov     = const_cast<struct iovec*>(iov);
        msg.msg_iovlen  = count;
        return ::recvmsg(rx_fd, &msg, 0);
    }

    static void cleanup(const Context& ctx) {
        // No filesystem cleanup required for network stack
    }
};

// --- POLICY 2: UNIX DOMAIN DGRAM ---
class SocketDgramPathPolicy {
 public:
    using Context = SocketDgramPathContext;

    static int init(Context& ctx, IoMode mode) {
        int fd = ::socket(AF_UNIX, SOCK_DGRAM, 0);
        if (fd == -1) {
            throw std::runtime_error("Unix DGRAM Socket creation failed");
        }

        if (mode == IoMode::RECEIVE_ONLY) {
            std::memset(&ctx.local_sockaddr, 0, sizeof(ctx.local_sockaddr));
            ctx.local_sockaddr.sun_family = AF_UNIX;
            std::strncpy(ctx.local_sockaddr.sun_path, ctx.local_path.c_str(), sizeof(ctx.local_sockaddr.sun_path) - 1);

            ::unlink(ctx.local_path.c_str());
            ::bind(fd, (sockaddr*)&ctx.local_sockaddr, sizeof(ctx.local_sockaddr));
            std::cout << "[Policy Unix] Generated Receiver FD: " << fd << std::endl;
        } else if (mode == IoMode::TRANSMIT_ONLY) {
            std::memset(&ctx.target_sockaddr, 0, sizeof(ctx.target_sockaddr));
            ctx.target_sockaddr.sun_family = AF_UNIX;
            std::strncpy(ctx.target_sockaddr.sun_path, ctx.target_path.c_str(),
                         sizeof(ctx.target_sockaddr.sun_path) - 1);
            ::connect(fd, (sockaddr*)&ctx.target_sockaddr, sizeof(ctx.target_sockaddr));
            std::cout << "[Policy Unix] Generated Transmitter FD: " << fd << std::endl;
        }

        return fd;
    }

    static ssize_t write_vector(int tx_fd, const struct iovec* iov, int count) { return ::writev(tx_fd, iov, count); }

    static ssize_t read_vector(int rx_fd, struct iovec* iov, int count) {
        struct sockaddr_storage sender_identity;
        std::memset(&sender_identity, 0, sizeof(sender_identity));
        struct msghdr msg;
        std::memset(&msg, 0, sizeof(msg));
        msg.msg_name    = &sender_identity;
        msg.msg_namelen = sizeof(sender_identity);
        msg.msg_iov     = const_cast<struct iovec*>(iov);
        msg.msg_iovlen  = count;
        return ::recvmsg(rx_fd, &msg, 0);
    }

    static void cleanup(const Context& ctx) { ::unlink(ctx.local_path.c_str()); }
};

// --- POLICY 3: UNIX DOMAIN STREAM ---
class SocketStreamPathPolicy {
 public:
    using Context = SocketStreamPathContext;

    static int init(Context& ctx, IoMode mode) {
        int fd = -1;
        // Critical: receiver::listen before transmiter::connect
        if (mode == IoMode::RECEIVE_ONLY) {
            fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd == -1) {
                throw std::runtime_error("Unix STREAM Receiver Socket creation failed");
            }

            std::memset(&ctx.local_sockaddr, 0, sizeof(ctx.local_sockaddr));
            ctx.local_sockaddr.sun_family = AF_UNIX;
            std::strncpy(ctx.local_sockaddr.sun_path, ctx.local_path.c_str(), sizeof(ctx.local_sockaddr.sun_path) - 1);

            ::unlink(ctx.local_path.c_str());
            ::bind(fd, (sockaddr*)&ctx.local_sockaddr, sizeof(ctx.local_sockaddr));

            ::listen(fd, 5);
            std::cout << "[Policy Unix STREAM] Generated Receiver FD and started listening: " << fd << std::endl;
        } else if (mode == IoMode::TRANSMIT_ONLY) {
            fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
            if (fd == -1) {
                throw std::runtime_error("Unix STREAM Transmitter Socket creation failed");
            }

            std::memset(&ctx.target_sockaddr, 0, sizeof(ctx.target_sockaddr));
            ctx.target_sockaddr.sun_family = AF_UNIX;
            std::strncpy(ctx.target_sockaddr.sun_path, ctx.target_path.c_str(),
                         sizeof(ctx.target_sockaddr.sun_path) - 1);

            ::connect(fd, (sockaddr*)&ctx.target_sockaddr, sizeof(ctx.target_sockaddr));
            std::cout << "[Policy Unix STREAM] Generated Transmitter FD and connected: " << fd << std::endl;
        }

        return fd;
    }

    static ssize_t write_vector(int tx_fd, const struct iovec* iov, int count) { return ::writev(tx_fd, iov, count); }

    static ssize_t read_vector(int rx_fd, struct iovec* iov, int count) { return ::readv(rx_fd, iov, count); }

    static void cleanup(const Context& ctx) { ::unlink(ctx.local_path.c_str()); }
};

template <typename Policy>
class UniversalSocket {
 private:
    // Automatically extracts the concrete context type associated with the Policy at compile-time!
    typename Policy::Context _ctx;

    int _rx_fd = -1;
    int _tx_fd = -1;

    std::atomic<bool> _is_running{true};
    std::thread       _receive_thread;

    void receive_loop() {
        std::cout << "[UniversalSocket] Async reader runtime queue loop active.\n";

        while (_is_running) {
            PacketHeader      header;
            std::vector<char> payload_buffer(1024);

            struct iovec iov[2];  // Properly configured static 2-element array
            iov[0].iov_base = &header;
            iov[0].iov_len  = sizeof(PacketHeader);
            iov[1].iov_base = payload_buffer.data();
            iov[1].iov_len  = payload_buffer.size();

            // Execute the exact isolated system call via the policy interface static command
            ssize_t received = Policy::read_vector(_rx_fd, iov, 2);

            if (received < 0) {
                if (!_is_running) break;
                std::cerr << "[UniversalSocket Error] Read execution failed.\n";
                break;
            }

            if (received >= static_cast<ssize_t>(sizeof(PacketHeader))) {
                uint32_t safe_size =
                    std::min(header.payload_size, static_cast<uint32_t>(received - sizeof(PacketHeader)));
                std::string content(payload_buffer.data(), safe_size);

                std::cout << "[UniversalSocket Receiver] Decoded -> Type: " << header.data_type
                          << " | Seq: " << header.seq << " | Size: " << header.payload_size << "\n"
                          << "[UniversalSocket Receiver] Content: " << content << "\n";
            }
        }
    }

 public:
    // === INIT STATE (Accepts the deduced Context type based on Policy template) ===
    UniversalSocket(const typename Policy::Context& context)
        : _ctx(context),                                     //
          _rx_fd(Policy::init(_ctx, IoMode::RECEIVE_ONLY)),  // Init receiver before transmiter
          _tx_fd(Policy::init(_ctx, IoMode::TRANSMIT_ONLY)) {
        // Fire background thread worker tasks
        _receive_thread = std::thread(&UniversalSocket::receive_loop, this);
    }

    // === CLOSE STATE ===
    ~UniversalSocket() {
        _is_running = false;
        if (_rx_fd >= 0) ::shutdown(_rx_fd, SHUT_RDWR);
        if (_tx_fd >= 0) ::shutdown(_tx_fd, SHUT_RDWR);

        if (_rx_fd >= 0) ::close(_rx_fd);
        if (_tx_fd >= 0) ::close(_tx_fd);

        if (_receive_thread.joinable()) {
            _receive_thread.join();
        }

        // Delegate specific system-wiping cleanup hook commands back to the policy
        Policy::cleanup(_ctx);
        std::cout << "[UniversalSocket] Scope closed cleanly. Resources completely recycled.\n";
    }
    // Unified lightning-fast zero-copy vector send routine
    template <typename T>
    bool send_packet(uint32_t data_type, const T& container, uint32_t seq = 0) {
        if (_tx_fd < 0 || !_is_running) return false;

        // Step 1: Initialize the protocol header block on the stack
        PacketHeader header{data_type, static_cast<uint32_t>(container.size()), seq};

        // Step 2: Define a strict 2-element array for Scatter-Gather Vector I/O
        struct iovec iov[2];

        // IOV Segment 0: Map the protocol header block
        iov[0].iov_base = &header;
        iov[0].iov_len  = sizeof(PacketHeader);

        // IOV Segment 1: Map the raw continuous underlying memory elements of the container
        iov[1].iov_base = const_cast<void*>(static_cast<const void*>(container.data()));
        iov[1].iov_len  = container.size() * sizeof(typename T::value_type);

        // Calculate the absolute combined bytes expected to be transmitted over the wire
        size_t total_expected = iov[0].iov_len + iov[1].iov_len;

        // Step 3: Perform static routing dispatch via the policy interface static command at compile-time
        ssize_t sent = Policy::write_vector(_tx_fd, iov, 2);

        return sent == static_cast<ssize_t>(total_expected);
    }
};

// ============================================================================
// APPLICATION DEPLOYMENT AND TESTING ENVIRONMENT
// ============================================================================
int main() {
    std::string payload  = "Advanced Policy-Based Desiip2_ctxgn architectural message.";
    std::string payload2 = "=== Hello === Messgage from ip2. ===";

    // ------------------------------------------------------------------------
    // SCENARIO 1: Deploying an IPv4 DGRAM Socket Node
    // ------------------------------------------------------------------------
    std::cout << "--- 1. INSTANTIATING SOCKET VIA IPV4 DGRAM POLICY ---\n";
    {
        // 1. Create the specific context structure for this network layout
        SocketDgramIPv4Context ip_ctx;
        ip_ctx.local_ip    = "127.0.0.1";
        ip_ctx.local_port  = 3333;
        ip_ctx.target_ip   = "127.0.0.1";
        ip_ctx.target_port = 9500;

        SocketDgramIPv4Context ip2_ctx;
        ip2_ctx.local_ip    = "127.0.0.1";
        ip2_ctx.local_port  = 9500;
        ip2_ctx.target_ip   = "127.0.0.1";
        ip2_ctx.target_port = 3333;

        // 2. Pass the Context directly into the Constructor of the flexible socket interface
        UniversalSocket<SocketDgramIPv4Policy> ip_node(ip_ctx);
        UniversalSocket<SocketDgramIPv4Policy> ip2_node(ip2_ctx);

        for (int i = 0; i < 20; i++) {
            std::string combine  = std::to_string(i) + " " + payload;
            std::string combine2 = std::to_string(i) + " " + payload2;
            ip_node.send_packet(99, combine, 666);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            ip2_node.send_packet(123, combine2, 888);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }  // Out of scope: Auto triggers clean destruction path

    // ------------------------------------------------------------------------
    // SCENARIO 2: Deploying a Local Unix Domain DGRAM Socket Node
    // ------------------------------------------------------------------------
    std::cout << "\n--- 2. INSTANTIATING SOCKET VIA UNIX DOMAIN PATH POLICY ---\n";
    {
        // 1. Create the filesystem context structure
        SocketDgramPathContext unix_ctx;
        unix_ctx.local_path  = "/tmp/strict_policy_A.sock";
        unix_ctx.target_path = "/tmp/strict_policy_A2.sock";

        SocketDgramPathContext unix2_ctx;
        unix_ctx.local_path  = "/tmp/strict_policy_A2.sock";
        unix_ctx.target_path = "/tmp/strict_policy_A.sock";

        // 2. Initialize the exact same unified interface using the Unix filesystem policy
        UniversalSocket<SocketDgramPathPolicy> unix_node(unix_ctx);
        UniversalSocket<SocketDgramPathPolicy> unix2_node(unix2_ctx);

        for (int i = 0; i < 20; i++) {
            std::string combine  = std::to_string(i) + " " + payload;
            std::string combine2 = std::to_string(i) + " " + payload2;
            unix_node.send_packet(99, combine, 333);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            unix2_node.send_packet(123, combine2, 999);
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }
    }  // Out of scope: Auto triggers clean destruction path and unlinks file nodes

    return 0;
}