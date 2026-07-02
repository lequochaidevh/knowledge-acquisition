#include "socket_client.h"
namespace HarisLinux {

// static auto logger = LogRegistry::getInstance().getLogger("SocketClient");

template <typename Transport>
bool SocketClient<Transport>::connect_to_server(const std::string& target, int port) {
    // logger->setLevel(LogLevel::Trace);

    if (!this->initialize_socket()) {
        HARIS_LOG_ERROR("[CheckLose] Initialize socket failed !");
        return false;
    };

    // Automatically configure local bind paths if client requires feedback or checking metrics
    if (this->_address_families == AF_UNIX && (this->_modes & (Ipc::Client::CheckLose | Ipc::Client::Feedback))) {
        sockaddr_un client_addr;
        std::memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sun_family = AF_UNIX;

        std::string client_path = "/tmp/uds_client_" + std::to_string(getpid()) + ".sock";
        ::unlink(client_path.c_str());

        std::strncpy(client_addr.sun_path, client_path.c_str(), sizeof(client_addr.sun_path) - 1);
        if (::bind(this->_socket_fd, reinterpret_cast<sockaddr*>(&client_addr), sizeof(sockaddr_un)) < 0) return false;
    }

    if (!this->configure_address(target, port)) return false;

    if (connect(this->_socket_fd,                                  //
                reinterpret_cast<sockaddr*>(&this->_remote_addr),  //
                this->_remote_addr_len) < 0)
        return false;

    return true;
}

template <typename Transport>
bool SocketClient<Transport>::pull_data(PacketHeader& out_header, std::vector<uint8_t>& out_payload) {
    if (!this->receive_packet(this->_socket_fd, out_header, out_payload)) return false;

    if (this->_modes & Ipc::Client::Feedback) {
        std::string ack_tag = "ACK_CLIENT";
        this->send_packet(this->_socket_fd, DataType::Heartbeat, ack_tag, out_header.sequence_id);
    }

    if ((this->_modes & Ipc::Client::CheckLose) && out_header.type == DataType::Heartbeat) {
        this->_sent_packets.erase(out_header.sequence_id);
    }

    return true;
}

template <typename Transport>
void SocketClient<Transport>::process_loss_monitoring() {
    if (!(this->_modes & Ipc::Client::CheckLose)) return;

    uint64_t                    now = get_current_timestamp_ms();
    std::lock_guard<std::mutex> lock(this->_map_mutex);

    auto it = this->_sent_packets.begin();
    while (it != this->_sent_packets.end()) {
        if (now - it->second > 200) {
            this->_lost_packets_count++;
            HARIS_LOG_WARN("[CheckLose] Lost Packet! Seq: {} | Total Lost: {}", it->first, this->_lost_packets_count);

            it = this->_sent_packets.erase(it);
        } else {
            ++it;
        }
    }
}

template <typename Transport>
void SocketClient<Transport>::check_lose_from_feedback() {
    PacketHeader ack_header;
    sockaddr_in  from_addr;
    socklen_t    addr_len = sizeof(from_addr);

    // try read Feedback ACK from server
    ssize_t received =
        recvfrom(this->_socket_fd, &ack_header, sizeof(PacketHeader), 0, (struct sockaddr*)&from_addr, &addr_len);

    HARIS_LOG_TRACE("Current map size tracking: {}", this->_sent_packets.size());

    if (received >= static_cast<ssize_t>(sizeof(PacketHeader))) {
        if (ack_header.type == DataType::Heartbeat) {
            // got ACK -> Remove it out of Client's queue
            HARIS_LOG_TRACE("ack_header.sequence_id = {}", static_cast<int>(ack_header.sequence_id));
            this->_sent_packets.erase(ack_header.sequence_id);
        }
    }

    // Check message over 200ms but can not get ACK -> Count Lose Increase
    uint64_t now = get_current_timestamp_ms();
    auto     it  = this->_sent_packets.begin();
    while (it != this->_sent_packets.end()) {
        if (now - it->second > 200) {  // over time
            this->_lost_packets_count++;

            HARIS_LOG_ERROR(
                "Lost Packet Detected! Seq ID:  {} "
                "| Total Lost: {} ",
                it->first, this->_lost_packets_count);

            it = this->_sent_packets.erase(it);
        } else {
            ++it;
        }
    }
}

// =================================================================
template class SocketClient<IIpc::StreamTag>;
template class SocketClient<IIpc::DgramTag>;

}  // namespace HarisLinux
