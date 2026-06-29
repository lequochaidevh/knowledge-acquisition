#pragma once
#include <cstdarg>

#include "logging/logger.h"
#include "ipc_sender.h"
#include "ipc_receiver.h"
#include "ipc_msg_dispatcher.h"

namespace HarisLinux {

namespace Ipc {
    struct StreamTag {};  // Identifies TCP / Named Pipes / SOCK_STREAM
    struct DgramTag {};   // Identifies UDP / SOCK_DGRAM
}  // namespace Ipc

// Helper Type Traits to determine if the Mode is Stream or Dgram
// (Adjust these structs to match your actual Ipc::Server/Client definitions)

template <typename Modes, typename Transport = Ipc::StreamTag>
class UnixSocket : public std::conditional_t<std::is_same_v<Transport, Ipc::StreamTag>, StreamSender, DgramSender>,
                   public std::conditional_t<std::is_same_v<Transport, Ipc::StreamTag>, StreamReceiver, DgramReceiver> {
 protected:
    // 1. Establish clean compile-time parent aliases
    using SenderBase   = std::conditional_t<std::is_same_v<Transport, Ipc::StreamTag>, StreamSender, DgramSender>;
    using ReceiverBase = std::conditional_t<std::is_same_v<Transport, Ipc::StreamTag>, StreamReceiver, DgramReceiver>;

 private:
    DECLARE_LOGGER;

 private:
    // 2. Factory method helper to instantiate the chosen Sender engine safely at compile time
    static constexpr SenderBase make_sender_base() {
        if constexpr (std::is_same_v<Transport, Ipc::StreamTag>) {
            return StreamSender(  // Constructs Stream variant
                UniqueFileDescriptor(-1, FileType::UnixSocket));
        } else {
            return DgramSender(  // Constructs Dgram variant
                UniqueFileDescriptor(-1, FileType::NetworkSocket, "/tmp/UnixSocket.sock"), "/tmp/UnixSocket.sock");
        }
    }

    static constexpr ReceiverBase make_receiver_base() {
        if constexpr (std::is_same_v<Transport, Ipc::StreamTag>) {
            return StreamReceiver(  // Constructs Stream variant
                UniqueFileDescriptor(-1, FileType::UnixSocket));
        } else {
            return DgramReceiver(  // Constructs Dgram variant
                UniqueFileDescriptor(-1, FileType::NetworkSocket));
        }
    }

 protected:
    int              _socket_fd;
    int              _address_families;  // Stores: AF_UNIX, AF_INET, AF_INET6, etc.
    int              _type;              // Stores: SOCK_DGRAM or SOCK_STREAM
    sockaddr_storage _remote_addr;       // Universal storage block large enough for any address family
    socklen_t        _remote_addr_len;   // Precise byte length of the active address structure

    uint32_t _sequence_counter;
    uint32_t _lost_packets_count;

    uint64_t get_current_timestamp_ms();

    std::mutex                   _map_mutex;
    std::map<uint32_t, uint64_t> _sent_packets;  // stored sequence_id and time to check timeout/lose

    Ipc::Generic<Modes> _modes;

 public:
    /**
     * @brief Single, adaptive unified constructor for Unix Network Sockets.
     * @note Completely bypasses direct naming constraints to ensure zero compilation drops.
     */
    UnixSocket(int address_families, int type, Ipc::Generic<Modes> modes)
        : SenderBase(make_sender_base()),      // Perfect forwarding execution via compile-time factory
          ReceiverBase(make_receiver_base()),  // Safely satisfies both StreamReceiver(-1) and DgramReceiver(-1)
          _address_families(address_families),
          _type(type),
          _remote_addr_len(sizeof(sockaddr_storage)),
          _modes(modes) {
        std::memset(&_remote_addr, 0, sizeof(_remote_addr));
        INIT_LOGGER("UnixSocket");
        logger->setLevel(LogLevel::Trace);
    }

    /**
     * @brief Destroys the socket object, safely releasing file descriptors.
     */
    virtual ~UnixSocket() {
        // if (ReceiverBase::_fd != -1) ::close(ReceiverBase::_fd);
        // if (SenderBase::_fd != -1) ::close(SenderBase::_fd);

        // if (_socket_fd != -1) {
        //     ::close(_socket_fd);
        // }
    }

    bool initialize_socket();
    // Maps target network descriptors into the generalized storage block abstraction layer
    bool configure_address(const std::string& target, int port = 0);
    int  get_socket_fd() const { return _socket_fd; }

    // void set_read_fd(int r_fd) { StreamReceiver::_fd = r_fd; }
    // void set_write_fd(int w_fd) { StreamSender::_fd = w_fd; }

    // int get_read_fd() const { return StreamReceiver::_fd; }
    // int get_write_fd() const { return StreamSender::_fd; }

    /**
     * @brief Transmits a structured data packet through a specific file descriptor.
     * @note Temporarily overrides the base implementation FD to route transactions seamlessly.
     *
     * @tparam T The payload data type.
     * @param write_fd The destination network file descriptor to write to.
     * @param type The classification tag tracking payload intent.
     * @param data The reference to the data structure being sent.
     * @param seq Unique sequence identification key for pipeline tracing.
     * @return true If the packet was successfully written to the system buffer.
     * @return false If the write operation failed or descriptor was invalid.
     */
    template <typename T>
    bool send_packet(int write_fd, DataType type, const T& data, uint32_t seq) {
        if (write_fd == -1) return false;

        // std::lock_guard<std::mutex> lock(_send_packet_mutex);

        /* Step 1: store main write fd before */
        UniqueFileDescriptor store_main_fd = std::move(SenderBase::_unique_fd);

        /* Step 2: Covert raw fd to smart fd */
        UniqueFileDescriptor unique_fd(write_fd, FileType::Pipe);
        SenderBase::_unique_fd = std::move(unique_fd);

        bool result = SenderBase::send(type, data, seq);

        /* Step 3: Send data with smart fd */
        if constexpr (std::is_same_v<Transport, Ipc::StreamTag>) {
            HARIS_LOG_DEBUG("------------ Socket Stream Send Data ------------");
        } else {
            HARIS_LOG_DEBUG("------------ Socket Dgram Send Data ------------");
        }

        /* Step 4: Release to not ::close raw write_fd */
        write_fd               = SenderBase::_unique_fd.release();
        SenderBase::_unique_fd = std::move(store_main_fd);
        return result;
    }

    /**
     * @brief Extracts and reassembles incoming structured frames from the network stream.
     */
    bool receive_packet(int source_fd, PacketHeader& out_header, std::vector<uint8_t>& out_payload);
};

}  // namespace HarisLinux
