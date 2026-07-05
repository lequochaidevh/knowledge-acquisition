#pragma once

#include "std17pch.h"
#include "logging/logger.h"

#include "ipc_sender.h"
#include "ipc_receiver.h"
#include "ipc_msg_dispatcher.h"

namespace HarisLinux {

template <typename Modes>
class PosixPipe : public IPCSenderBase<PipePolicy>, public StreamReceiver {
 private:
    DECLARE_LOGGER;

    std::mutex _send_packet_mutex;
    std::mutex _read_packet_mutex;

 public:
    /** @brief Get fd with int for Linux API*/
    int const get_read_fd() const { return StreamReceiver::_unique_fd.get(); }
    int const get_write_fd() const { return IPCSenderBase<PipePolicy>::_shared_fd.get(); }

    bool set_write_fd(int write_fd) {
        SharedFileDescription<PipePolicy> shared_proxy;
        shared_proxy                          = SharedFileDescription<PipePolicy>(write_fd);
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
        : IPCSenderBase<PipePolicy>(SharedFileDescription<PipePolicy>{}),
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
    bool send_packet(DataType type, const T& data, const uint32_t& seq = 0) const {
        // Triggers the atomic zero-copy layout executed entirely on stack via IPCSender
        bool result = IPCSenderBase<PipePolicy>::send(type, data, seq);
        HARIS_LOG_DEBUG("------------ Derived Send Data ------------");
        return result;
    }

    // Adapt function : todo remove it
    // Todo:
    template <typename T>
    bool send_packet(SharedFileDescription<PipePolicy>& shared_proxy_fd, DataType type, const T& data,
                     const uint32_t& seq = 0) {
        if (!shared_proxy_fd) return false;

        // Step 1: Process scatter-gather layout pointers natively on the execution stack frame
        const uint8_t* payload_ptr  = nullptr;
        uint32_t       payload_size = 0;
        using CleanedType           = std::decay_t<T>;

        if constexpr (std::is_arithmetic_v<CleanedType>) {
            payload_ptr  = reinterpret_cast<const uint8_t*>(&data);
            payload_size = static_cast<uint32_t>(sizeof(T));
        } else if constexpr (std::is_same_v<CleanedType, std::string> ||
                             std::is_same_v<CleanedType, std::vector<uint8_t>> ||
                             std::is_same_v<CleanedType, std::string_view>) {
            payload_ptr  = reinterpret_cast<const uint8_t*>(data.data());
            payload_size = static_cast<uint32_t>(data.size() * sizeof(typename CleanedType::value_type));
        } else if constexpr (std::is_same_v<CleanedType, IPCRequestPayload>) {
            payload_ptr  = reinterpret_cast<const uint8_t*>(&data);
            payload_size = static_cast<uint32_t>(sizeof(T));
        } else {
            static_assert(sizeof(T) == 0, "Unsupported compile-time type execution for IPC Sender!");
        }

        PacketHeader header{type, payload_size, get_current_timestamp_ms(), seq};

        struct iovec iov[2];

        // Index 0 stores the packet header metadata block cleanly
        iov[0].iov_base = const_cast<PacketHeader*>(&header);
        iov[0].iov_len  = sizeof(PacketHeader);

        // Index 1 stores the variable length payload buffer block cleanly
        iov[1].iov_base = const_cast<uint8_t*>(payload_ptr);
        iov[1].iov_len  = payload_size;

        size_t total_bytes = sizeof(PacketHeader) + payload_size;

        auto session = shared_proxy_fd.lock();

        // Step 3: Execute the compiled vector write through the policy layer natively
        ssize_t sent = PipePolicy::write_vector(session.get_fd(), iov, shared_proxy_fd.get_context());

        // STEP 4: CLEAN DISENGAGEMENT
        // Safely unlinks from the registry and resets internal state to -1 for the next call
        // SharedFileDescription<PipePolicy>::release(write_fd);

        return sent == static_cast<ssize_t>(total_bytes);
        // SharedFileDescription<PipePolicy> temporary = _shared_fd;
        // _shared_fd                                  = shared_proxy_fd;
        // bool result                                 = this->template send_packet<T>(type, data, seq);
        // _shared_fd                                  = temporary;
        // return result;
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
