#pragma once

#include "std17pch.h"
#include "logging/logger.h"

#include "ipc_sender.h"
#include "ipc_receiver.h"
#include "ipc_msg_dispatcher.h"

namespace HarisLinux {

template <typename Modes>
class PosixPipe : public IPCSenderBase<PipePolicy>, public IPCReceiverBase<PipePolicy> {
 private:
    DECLARE_LOGGER;

 public:
    // TODO REMOVED
    /** @brief Get fd with int for Linux API*/
    SharedFileDescriptor<PipePolicy> const get_read_sfd() const  //
    {
        return IPCSenderBase<PipePolicy>::_shared_fd;
    }

    SharedFileDescriptor<PipePolicy> const get_write_sfd() const {  //
        return IPCSenderBase<PipePolicy>::_shared_fd;
    }

    bool set_read_sfd(int read_fd) {
        SharedFileDescriptor<PipePolicy> shared_proxy;
        shared_proxy                          = SharedFileDescriptor<PipePolicy>(read_fd);
        IPCSenderBase<PipePolicy>::_shared_fd = shared_proxy;
        return true;
    }

    bool set_write_sfd(int write_fd) {
        SharedFileDescriptor<PipePolicy> shared_proxy;
        shared_proxy                          = SharedFileDescriptor<PipePolicy>(write_fd);
        IPCSenderBase<PipePolicy>::_shared_fd = shared_proxy;
        return true;
    }

 protected:
    /** @brief File system path for the pipe.
     * Data with transmit from client to server */
    std::string _upstream_path;

    /** @brief File system path for the pipe.
     * Data with transmit from server to client*/
    std::string _downstream_path;

    /** @brief Configuration modes for the IPC channel.
     * exam 1: Ipc::Generic server_flags = Ipc::Server::Feedback | Ipc::Server::CheckLose;
     * exam 2: Ipc::Generic client_flags = Ipc::Client::Feedback | Ipc::Client::CheckLose;
     */
    Ipc::Generic<Modes> _modes;

    /**
     * @brief Constructs a bidirectional POSIX Named Pipe channel.
     * @param base_path The base file path used to generate the specialized pipe paths.
     * @param modes Operational modes for this IPC instance.
     */
    PosixPipe(const std::string& path, Ipc::Generic<Modes> modes)
        : IPCSenderBase<PipePolicy>(SharedFileDescriptor<PipePolicy>{}),
          IPCReceiverBase<PipePolicy>(SharedFileDescriptor<PipePolicy>{}),
          _upstream_path(path),
          _downstream_path(path + "_fb"),  // Retained suffix for physical file mapping
          _modes(modes) {
        INIT_LOGGER("PosixPipe");
        logger->setLevel(LogLevel::Trace);
    }

    virtual ~PosixPipe() {}

    /**
     * @brief Transmits a structured data packet through a specific file descriptor.
     * @note Temporarily overrides the default sender FD to ensure zero-copy/atomic write operations.
     *
     * @tparam T The payload data type.
     * @param type The categorization type of the data packet.
     * @param data The reference to the data structure being sent.
     * @param seq Optional sequence number for packet ordering and tracking.
     * @return true If the packet was successfully written to the stream.
     * @return false If the write operation failed.
     */
    template <typename T>
    bool send_packet(DataType type, const T& data, const uint32_t& seq = 0) const {
        // Triggers the atomic zero-copy layout executed entirely on stack via IPCSender
        HARIS_LOG_DEBUG("------------ Derived Send Data ------------");
        return IPCSenderBase<PipePolicy>::send(type, data, seq);
    }

    template <typename T>
    bool send_packet(SharedFileDescriptor<PipePolicy>& shared_proxy_fd, DataType type, const T& data,
                     const uint32_t& seq = 0) {
        if (!shared_proxy_fd) {
            HARIS_LOG_CRITICAL("Invalid share posix fd");
            return false;
        }

        SharedFdSwitchGuard<PipePolicy> guard(IPCSenderBase<PipePolicy>::_shared_fd, shared_proxy_fd);

        bool result = this->template send_packet<T>(type, data, seq);
        return result;
    }

    /**
     * @brief Receives and parses an incoming packet from the specified channel.
     *
     * @param[out] header Output structure to store the parsed packet header metadata.
     * @param[out] payload Output byte vector to store the raw packet payload.
     * @return true If a valid packet was successfully read and decoded.
     * @return false If a read error occurred or the packet was corrupted.
     */
    bool receive_packet(PacketHeader& header, std::vector<uint8_t>& payload);

    // Adapt function : todo remove it
    bool receive_packet(SharedFileDescriptor<PipePolicy>& shared_proxy_fd, PacketHeader& header,
                        std::vector<uint8_t>& payload) {
        if (!shared_proxy_fd) {
            HARIS_LOG_CRITICAL("Invalid share posix fd");
            return false;
        }

        // Pass the vector element directly by reference without making any temporal copy copies
        SharedFdSwitchGuard<PipePolicy> guard(IPCReceiverBase<PipePolicy>::_shared_fd, shared_proxy_fd);

        bool result = this->receive_packet(header, payload);
        return result;
    }

    bool send_heartbeat(SharedFileDescriptor<PipePolicy>& shared_proxy_fd,  //
                        const std::string& self_id, uint32_t seq) {
        // Transient blank dummy vector used to satisfy your base send method constraints
        PacketHeader heartbeat_header{};
        heartbeat_header.type         = DataType::Heartbeat;
        heartbeat_header.payload_size = sizeof(self_id);  // <--- Zero payload allocation footprint!
        heartbeat_header.timestamp_ms = get_current_timestamp_ms();
        heartbeat_header.sequence_id  = seq;

        // Executes the lockless context switch guard and fires down the pipe channel
        return send_packet(shared_proxy_fd, DataType::Heartbeat, self_id, seq);
    }
};

}  // namespace HarisLinux
