#pragma once
#include "unix_socket.h"

namespace HarisLinux {

template <typename Transport = IIpc::StreamTag>
class SocketClient : public UnixSocket<Ipc::Client, Transport> {
    DECLARE_LOGGER;

 private:
    // 1. Define alias
    using Base         = UnixSocket<Ipc::Client, Transport>;
    using SenderBase   = typename Base::SenderBase;  // StreamSender
    using ReceiverBase = typename Base::ReceiverBase;

 public:
    // Explicitly chains base constructor bindings to pipe parameters down
    SocketClient(int address_families, int type, Ipc::Generic<Ipc::Client> modes)
        : Base(address_families, type, modes) {}

    // Add a custom destructor to your SocketClient class to clear file descriptors cleanly
    virtual ~SocketClient() {
        // Unlink the custom temporary file system node to prevent polluting /tmp directory
        std::string client_path = "/tmp/uds_client_" + std::to_string(getpid()) + ".sock";
        ::unlink(client_path.c_str());
    }

    // Attaches the client endpoint node to target listeners
    bool connect_to_server(const std::string& target, int port = 0);

    // Client Send Wrapper
    template <typename T>
    bool push_data(DataType type, const T& data) {
        uint32_t current_seq = this->_sequence_counter++;

        if (this->_modes & Ipc::Client::CheckLose) {
            std::lock_guard<std::mutex> lock(this->_map_mutex);
            this->_sent_packets[current_seq] = get_current_timestamp_ms();
        }

        bool success = this->send_packet(this->_socket_fd, type, data, current_seq);

        if (!success && (this->_modes & Ipc::Client::CheckLose)) {
            std::lock_guard<std::mutex> lock(this->_map_mutex);
            this->_sent_packets.erase(current_seq);
        }
        return success;
    }

    // Client Receive Wrapper (Enables checking for feedback ACKs or tracking heartbeats)
    bool pull_data(PacketHeader& out_header, std::vector<uint8_t>& out_payload);

    void process_loss_monitoring();
    // Check feedback from Server (Check Lose From Feedback)
    void check_lose_from_feedback();
};

}  // namespace HarisLinux
