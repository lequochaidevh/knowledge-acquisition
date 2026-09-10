#pragma once
#include "IOInterface.h"
#include <atomic>
#include <thread>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

class UdpTransport : public IOInterface {
 private:
    int                _sockfd = -1;
    struct sockaddr_in _target_addr {};
    std::thread        _recv_thread;
    std::atomic<bool>  _is_running{false};

    void receive_loop() {
        uint8_t            buffer[65535];
        struct sockaddr_in src_addr {};
        socklen_t          addr_len = sizeof(src_addr);

        while (_is_running) {
            ssize_t bytes_received =
                recvfrom(_sockfd, buffer, sizeof(buffer), 0, (struct sockaddr*)&src_addr, &addr_len);

            if (bytes_received < 0) {
                if (_is_running) {
                    std::cerr << "[UdpTransport] Error receiving data\n";
                }
                break;
            }

            if (bytes_received > 0 && _data_callback) {
                _data_callback(buffer, static_cast<size_t>(bytes_received));
            }
        }
    }

 public:
    UdpTransport() = default;

    ~UdpTransport() override { disconnect(); }

    bool connect(const std::string& target_ip, uint16_t port) override {
        _sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (_sockfd < 0) {
            std::cerr << "[UdpTransport] Failed to create socket\n";
            return false;
        }

        // Configure socket to allow address reuse
        int opt = 1;
        setsockopt(_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        std::memset(&_target_addr, 0, sizeof(_target_addr));
        _target_addr.sin_family = AF_INET;
        _target_addr.sin_port   = htons(port);

        if (inet_pton(AF_INET, target_ip.c_str(), &_target_addr.sin_addr) <= 0) {
            std::cerr << "[UdpTransport] Invalid IP address\n";
            close(_sockfd);
            return false;
        }

        // Bind locally so it can also listen/receive packets on this port
        struct sockaddr_in local_addr {};
        std::memset(&local_addr, 0, sizeof(local_addr));
        local_addr.sin_family      = AF_INET;
        local_addr.sin_addr.s_addr = INADDR_ANY;
        local_addr.sin_port        = htons(port);

        if (bind(_sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
            std::cerr << "[UdpTransport] Bind failed\n";
            close(_sockfd);
            return false;
        }

        _is_running  = true;
        _recv_thread = std::thread(&UdpTransport::receive_loop, this);
        std::cout << "[UdpTransport] Connected and listening on port " << port << "\n";
        return true;
    }

    void disconnect() override {
        _is_running = false;
        if (_sockfd >= 0) {
            // Shutdown socket to unblock recvfrom immediately
            shutdown(_sockfd, SHUT_RDWR);
            close(_sockfd);
            _sockfd = -1;
        }
        if (_recv_thread.joinable()) {
            _recv_thread.join();
        }
        std::cout << "[UdpTransport] Disconnected cleanly\n";
    }

    bool send(const uint8_t* data, size_t size) override {
        if (_sockfd < 0) return false;

        ssize_t bytes_sent = sendto(_sockfd, data, size, 0, (struct sockaddr*)&_target_addr, sizeof(_target_addr));
        return bytes_sent == static_cast<ssize_t>(size);
    }
};