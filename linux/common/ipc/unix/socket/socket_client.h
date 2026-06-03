#pragma once
#include "unix_socket.h"

namespace HarisLinux {

class SocketClient : public UnixSocket {
 private:
    uint32_t _lost_packets_count;

 public:
    SocketClient() : UnixSocket(), _lost_packets_count(0) {}

    bool connect_to_server(const std::string& ip, int port);

    // Check feedback from Server (Check Lose From Feedback)
    void check_lose_from_feedback();
};

}  // namespace HarisLinux
