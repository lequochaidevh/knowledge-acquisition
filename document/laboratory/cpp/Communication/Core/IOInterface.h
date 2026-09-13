#pragma once
#include <vector>
#include <cstdint>
#include <functional>
#include <string>

class IOInterface {
 public:
    using DataCallback = std::function<void(const uint8_t* data, size_t size)>;

 protected:
    DataCallback _data_callback;

 public:
    virtual ~IOInterface() = default;

    virtual bool connect(const std::string& target, uint16_t port) = 0;
    virtual void disconnect()                                      = 0;
    virtual bool send(const uint8_t* data, size_t size)            = 0;

    void register_read_callback(DataCallback&& cb) { _data_callback = std::move(cb); }
};