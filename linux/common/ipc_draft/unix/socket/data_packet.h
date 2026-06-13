#pragma once
#include "ipc_metadata.h"

#include "logging/logger.h"
#include <iomanip>
#include <type_traits>

namespace HarisLinux {

struct SocketUtils {
    template <typename T>
    static T unpack(const std::vector<uint8_t>& payload) {
        if constexpr (std::is_same_v<T, std::string>) {
            return std::string(reinterpret_cast<const char*>(payload.data()), payload.size());
        } else if constexpr (std::is_same_v<T, std::vector<uint8_t>>) {
            return payload;
        } else if constexpr (std::is_trivially_copyable_v<T>) {
            T result{};
            if (payload.size() >= sizeof(T)) {
                std::memcpy(&result, payload.data(), sizeof(T));
            }
            return result;
        } else {
            static_assert(sizeof(T) == 0, "Unsupported compile-time type extraction constraint!");
        }
    }
};

class DataHandlerPolicy {
 public:
    virtual void on_packet(const std::string& text) {
        std::cout << ">> [Template Handler] Text Received: \"" << text << "\"\n";
    }

    virtual void on_packet(double num) { std::cout << ">> [Template Handler] Double Received: " << num << "\n"; }

    virtual void on_packet(int num) { std::cout << ">> [Template Handler] Int Received: " << num << "\n"; }

    virtual void on_packet(const std::vector<uint8_t>& media) {
        std::cout << ">> [Template Handler] Media Buffer Size: " << media.size() << " bytes\n";
        if (!media.empty()) {
            std::cout << ">> Bytes (Hex): ";
            for (auto b : media) {
                std::printf("%02X ", b);
            }
            std::cout << "\n";
        }
    }

    virtual void on_packet(const IPCRequestPayload& cmd) {
        std::cout << ">> [Template Handler] Command Payload -> ID: " << cmd.client_id << " | Cmd: " << cmd.command
                  << "\n";
    }

    DataHandlerPolicy() {}
    virtual ~DataHandlerPolicy() {}
};

template <typename HandlerPolicy>
class PacketDispatcher {
 private:
    HandlerPolicy _handler;

 public:
    void dispatch(const PacketHeader& header, const std::vector<uint8_t>& payload) {
        switch (header.type) {
            case DataType::Heartbeat: {
                _handler.on_packet(SocketUtils::unpack<std::string>(payload));
                break;
            }
            case DataType::Text: {
                // cast at compile-time
                _handler.on_packet(SocketUtils::unpack<std::string>(payload));
                break;
            }
            case DataType::Number: {
                if (payload.size() == sizeof(double)) {
                    _handler.on_packet(SocketUtils::unpack<double>(payload));
                } else if (payload.size() == sizeof(int)) {
                    _handler.on_packet(SocketUtils::unpack<int>(payload));
                }
                break;
            }
            case DataType::Media: {
                _handler.on_packet(SocketUtils::unpack<std::vector<uint8_t>>(payload));
                break;
            }
            case DataType::Command: {
                _handler.on_packet(SocketUtils::unpack<IPCRequestPayload>(payload));
                break;
            }
            default:
                std::cerr << ">> Unknown DataType received in dispatcher pipeline.\n";
                break;
        }
    }
};

}  // namespace HarisLinux
