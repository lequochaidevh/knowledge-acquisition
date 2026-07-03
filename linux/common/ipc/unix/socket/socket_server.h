#pragma once
#include "unix_socket.h"

namespace HarisLinux {

template <typename Transport = SocketType::StreamTag>
class SocketServer : public UnixSocket<Ipc::Server, Transport> {
 private:
    DECLARE_LOGGER;
    // 1. Define alias
    using Base         = UnixSocket<Ipc::Server, Transport>;
    using SenderBase   = typename Base::SenderBase;  // StreamSender
    using ReceiverBase = typename Base::ReceiverBase;

    int _client_fd = -1;  // Specific stream context channel (active only under SOCK_STREAM)

    std::vector<sockaddr_storage> _connected_clients;  // Tracks active client addresses for Broadcasting

    // Registers unique endpoints into our delivery table during incoming Datagram operations
    void add_client_if_new(const sockaddr_storage& client_addr, socklen_t addr_len);
    // Broadcast)

 public:
    SocketServer(int address_families, int type, Ipc::Generic<Ipc::Server> modes)
        : Base(address_families, type, modes) {}

    virtual ~SocketServer() {
        if (_client_fd != -1) close(_client_fd);
    }

    bool start_server(const std::string& target, int port = 0);

    bool accept_connection();

    void process_loss_monitoring();

    // API sort: get all data.
    bool pull_data(PacketHeader& out_header, std::vector<uint8_t>& out_payload);

    // Server Send Wrapper (Enables mismatch feedback loop checks back to client)
    template <typename T>
    bool push_data(DataType type, const T& data) {
        int fd = (this->_type == SOCK_STREAM) ? _client_fd : this->_socket_fd;

        // Directly use your class counter member to assign the packet ID
        // without declaring a conflicting local 'uint32_t seq' variable.
        uint32_t current_seq = this->_sequence_counter++;

        // DYNAMIC CHECK: Is the CheckLose bit active?
        if (this->_modes & Ipc::Server::CheckLose) {
            std::lock_guard<std::mutex> lock(this->_map_mutex);
            this->_sent_packets[current_seq] = get_current_timestamp_ms();
        }

        // Pass 'current_seq' safely down to the base sender
        bool success = this->send_packet(fd, type, data, current_seq);  // or Base::send_packet

        if (!success && (this->_modes & Ipc::Server::CheckLose)) {
            std::lock_guard<std::mutex> lock(this->_map_mutex);
            this->_sent_packets.erase(current_seq);
        }
        return success;
    }

    // MODE EVALUATION 3: Broadcast Mode
    // Symmetrical Extension: Dispatches a message to all tracked client endpoints
    // =========================================================================
    // BROADCAST PIPELINE WITH BITMASK FLAGS
    // Dispatches a message to all discovered or connected endpoints
    // =========================================================================
    template <typename T>
    void broadcast_packet(DataType type, const T& data, uint32_t& seq_counter) {
        if (!(this->_modes & Ipc::Server::Broadcast)) return;

        // Stream Profile: Direct communication pipeline to the actively accepted descriptor
        if (this->_type == SOCK_STREAM) {
            if (_client_fd != -1) {
                this->send_packet(_client_fd, type, data, seq_counter++);
            }
            return;
        }

        // Datagram Profile: Safe sweep loop across the registered endpoint allocation map table
        if (_connected_clients.empty()) return;

        // Back up active contextual remote addresses to prevent breaking concurrent flow tracking
        sockaddr_storage backup_addr = this->_remote_addr;
        socklen_t        backup_len  = this->_remote_addr_len;

        for (const auto& client : _connected_clients) {
            this->_remote_addr = client;
            this->_remote_addr_len =
                (this->_address_families == AF_UNIX)
                    ? sizeof(sockaddr_un)
                    : (this->_address_families == AF_INET) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);

            // Fire batch data structures seamlessly out of the socket descriptor
            this->send_packet(this->_socket_fd, type, data, seq_counter++);
        }

        // Restore active operational context parameters safely back to memory maps
        this->_remote_addr     = backup_addr;
        this->_remote_addr_len = backup_len;
    }
};

}  // namespace HarisLinux
