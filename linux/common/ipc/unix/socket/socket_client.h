#pragma once
#include "unix_socket.h"

namespace HarisLinux {

class SocketClient : public UnixSocket {
 private:
    const Ipc::Client _modes;  // Re-integrated: Write/Read-Only, Feedback, Checklose

 public:
    // Explicitly chains base constructor bindings to pipe parameters down
    SocketClient(int address_families, int type, Ipc::Client modes)
        : UnixSocket(address_families, type), _modes(modes) {}

    // Add a custom destructor to your SocketClient class to clear file descriptors cleanly
    virtual ~SocketClient() {
        // Unlink the custom temporary file system node to prevent polluting /tmp directory
        std::string client_path = "/tmp/uds_client_" + std::to_string(getpid()) + ".sock";
        unlink(client_path.c_str());
    }

    // Attaches the client endpoint node to target listeners
    bool connect_to_server(const std::string& target, int port = 0);

    // Client Send Wrapper
    template <typename T>
    bool send_packet(DataType type, const T& data) {
        uint32_t current_seq = _sequence_counter++;

        if (_modes & Ipc::Client::CheckLose) {
            std::lock_guard<std::mutex> lock(_map_mutex);
            _sent_packets[current_seq] = get_current_timestamp_ms();
        }

        bool success = base_send(_socket_fd, type, data, current_seq);

        if (!success && (_modes & Ipc::Client::CheckLose)) {
            std::lock_guard<std::mutex> lock(_map_mutex);
            _sent_packets.erase(current_seq);
        }
        return success;
    }

    // Client Receive Wrapper (Enables checking for feedback ACKs or tracking heartbeats)
    bool receive_packet(PacketHeader& out_header, std::vector<uint8_t>& out_payload);

    void process_loss_monitoring();
    // Check feedback from Server (Check Lose From Feedback)
    void check_lose_from_feedback();
};

}  // namespace HarisLinux
