#pragma once

#include "std17pch.h"
#include "logging/logger.h"

#include "ipc_sender.h"
#include "ipc_receiver.h"
#include "ipc_msg_dispatcher.h"

namespace HarisLinux {

template <typename Modes>
class PosixPipe : public StreamSender, public StreamReceiver {
 public:
    int get_read_fd() const { return StreamReceiver::_fd; }
    int get_write_fd() const { return StreamSender::_fd; }

 private:
    DECLARE_LOGGER;

 protected:
    int _read_fd  = -1;
    int _write_fd = -1;

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
        : StreamSender(-1),
          StreamReceiver(-1),
          _upstream_path(path),
          _downstream_path(path + "_fb"),  // Retained suffix for physical file mapping
          _modes(modes) {
        INIT_LOGGER("PosixPipe");
        logger->setLevel(LogLevel::Trace);
    }

    virtual ~PosixPipe() {
        if (StreamReceiver::_fd != -1) ::close(StreamReceiver::_fd);
        if (StreamSender::_fd != -1) ::close(StreamSender::_fd);

        if (_read_fd != -1) close(_read_fd);
        if (_write_fd != -1) close(_write_fd);
    }

    void set_read_fd(int r_fd) { StreamReceiver::_fd = r_fd; }
    void set_write_fd(int w_fd) { StreamSender::_fd = w_fd; }

    /**
     * @brief Transmits a structured data packet through a specific file descriptor.
     * @note Temporarily overrides the default sender FD to ensure zero-copy/atomic write operations.
     *
     * @tparam T The payload data type.
     * @param write_fd The destination file descriptor to write to.
     * @param type The categorization type of the data packet.
     * @param data The reference to the data structure being sent.
     * @param seq Optional sequence number for packet ordering and tracking.
     * @return true If the packet was successfully written to the stream.
     * @return false If the write operation failed.
     */
    template <typename T>
    bool send_packet(int write_fd, DataType type, const T& data, const uint32_t& seq = 0) {
        // core logic writev
        int old_fd        = StreamSender::_fd;
        StreamSender::_fd = write_fd;

        // (Zero-copy / Atomic)
        bool result = StreamSender::send(type, data, seq);
        HARIS_LOG_DEBUG("------------ Derived Send Data ------------");

        StreamSender::_fd = old_fd;

        return result;
    }

    /**
     * @brief Receives and parses an incoming packet from the specified channel.
     *
     * @param read_fd The source file descriptor to read from.
     * @param[out] header Output structure to store the parsed packet header metadata.
     * @param[out] payload Output byte vector to store the raw packet payload.
     * @return true If a valid packet was successfully read and decoded.
     * @return false If a read error occurred or the packet was corrupted.
     */
    bool receive_packet(int read_fd, PacketHeader& header, std::vector<uint8_t>& payload);
};

}  // namespace HarisLinux
