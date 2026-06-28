#pragma once

#include "std17pch.h"
#include "logging/logger.h"

#include "ipc_sender.h"
#include "ipc_receiver.h"
#include "ipc_msg_dispatcher.h"

namespace HarisLinux {

template <typename Modes>
class PosixPipe : public StreamSender, public StreamReceiver {
 private:
    DECLARE_LOGGER;

    std::mutex _read_packet_mutex;

 public:
    /** @brief Get fd with int for Linux API*/
    int get_read_fd() const { return StreamReceiver::_unique_fd.get(); }
    int get_write_fd() const { return StreamSender::_unique_fd.get(); }

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
        : StreamSender(UniqueFileDescriptor(-1, FileType::Pipe)),
          StreamReceiver(UniqueFileDescriptor(-1, FileType::Pipe)),
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
    bool send_packet(DataType type, const T& data, const uint32_t& seq = 0) {
        // (Zero-copy / Atomic)
        bool result = StreamSender::send(type, data, seq);
        HARIS_LOG_DEBUG("------------ Derived Send Data ------------");
        return result;
    }

    // Adapt function : todo remove it
    // Todo:
    template <typename T>
    bool send_packet(int write_fd, DataType type, const T& data, const uint32_t& seq = 0) {
        if (write_fd < 0) return false;

        /* Step 1: store main write fd before */
        UniqueFileDescriptor store_main_fd = std::move(StreamSender::_unique_fd);

        /* Step 2: Covert raw fd to smart fd */
        UniqueFileDescriptor unique_fd(write_fd, FileType::Pipe);
        StreamSender::_unique_fd = std::move(unique_fd);

        /* Step 3: Send data with smart fd */
        bool result = this->template send_packet<T>(type, data, seq);

        /* Step 4: Release to not ::close raw write_fd */
        write_fd                 = StreamSender::_unique_fd.release();
        StreamSender::_unique_fd = std::move(store_main_fd);

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
    bool receive_packet(int read_fd, PacketHeader& header, std::vector<uint8_t>& payload) {
        if (read_fd < 0) return false;

        std::lock_guard<std::mutex> lock(_read_packet_mutex);

        /* Step 1: store main write fd before */
        UniqueFileDescriptor store_main_fd = std::move(StreamReceiver::_unique_fd);

        /* Step 2: Covert raw fd to smart fd */
        UniqueFileDescriptor unique_fd(read_fd, FileType::Pipe);
        StreamReceiver::_unique_fd = std::move(unique_fd);

        /* Step 3: Send data with smart fd */
        bool result = receive_packet(header, payload);
        if (!result) HARIS_LOG_ERROR("Got packet failed");
        /* Step 4: Release to not ::close raw read_fd */
        read_fd                    = StreamReceiver::_unique_fd.release();
        StreamReceiver::_unique_fd = std::move(store_main_fd);

        return result;
    }
};

}  // namespace HarisLinux
