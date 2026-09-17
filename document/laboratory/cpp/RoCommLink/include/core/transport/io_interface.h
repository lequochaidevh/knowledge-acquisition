#pragma once
#include "std17pch.h"
#include <cerrno>

class IOInterface {
 public:
    using DataCallback = std::function<void(std::string_view data)>;

 protected:
    DataCallback _data_callback;

 public:
    virtual ~IOInterface() = default;

    //  virtual bool connect(const std::string& target, uint16_t port)                             = 0;
    virtual bool connect(const std::string& target, uint16_t local_port, uint16_t remote_port) = 0;

    virtual void disconnect()                = 0;
    virtual bool send(std::string_view data) = 0;

    void register_read_callback(DataCallback&& cb) { _data_callback = std::move(cb); }
};
