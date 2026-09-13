#pragma once
#include "io_interface.h"

class UdpTransport : public IOInterface {
 private:
    int                _sockfd = -1;
    struct sockaddr_in _target_addr {};
    std::thread        _recv_thread;
    std::atomic<bool>  _is_running{false};

    void receive_loop();

 public:
    UdpTransport() = default;

    ~UdpTransport() override;

    bool connect(const std::string& target_ip, uint16_t port) override;

    void disconnect() override;

    bool send(const uint8_t* data, size_t size) override;
};