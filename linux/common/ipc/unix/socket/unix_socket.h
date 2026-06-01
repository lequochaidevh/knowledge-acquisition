#pragma once
#include "socket_metadata.h"
#include <chrono>

class UnixSocket {
 protected:
    int         _socket_fd;
    sockaddr_in _server_addr;

    uint64_t get_current_timestamp_ms();

 public:
    UnixSocket() : _socket_fd(-1) { std::memset(&_server_addr, 0, sizeof(_server_addr)); }

    virtual ~UnixSocket() {
        if (_socket_fd != -1) {
            close(_socket_fd);
        }
    }

    bool initialize_socket();
};
