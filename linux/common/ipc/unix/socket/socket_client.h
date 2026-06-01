#pragma once
#include "unix_socket.h"

class SocketClient : public UnixSocket {
 private:
    uint32_t                     _sequence_counter;
    uint32_t                     _lost_packets_count;
    std::map<uint32_t, uint64_t> _sent_packets;  // stored sequence_id and time to check timeout/lose

 public:
    SocketClient() : _sequence_counter(0), _lost_packets_count(0) {}

    bool connect_to_server(const std::string& ip, int port);

    bool send_packet(DataType type, const uint8_t* data, uint32_t size);

    bool send_number(double num);

    bool send_text(const std::string& text);

    // Check feedback from Server (Check Lose From Feedback)
    void check_lose_from_feedback();
};
