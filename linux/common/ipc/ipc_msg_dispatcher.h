#pragma once
#include "ipc_metadata.h"

#include "logging/logger.h"
#include <iomanip>
#include <type_traits>

namespace HarisLinux {

struct IpcUtils {
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
 private:
    DECLARE_LOGGER;

 public:
    DataHandlerPolicy(const std::string& module) { INIT_LOGGER(module); };
    virtual ~DataHandlerPolicy(){};

    void on_packet(const std::string& text) { HARIS_LOG_DEBUG("Data: \"{}\"", text); }

    void on_packet(double num) { HARIS_LOG_DEBUG("Data: [Double] {}", num); }

    void on_packet(int num) { HARIS_LOG_DEBUG("Data: [Int] {}", num); }

    void on_packet(const std::vector<uint8_t>& media) {
        HARIS_LOG_DEBUG("Media Buffer Size: {} bytes", media.size());

        if (logger->getLevel() != LogLevel::Trace) return;
        if (!media.empty()) {
            // stack/heap smart memory
            fmt::memory_buffer out;

            // Append prefix char
            fmt::format_to(std::back_inserter(out), ">> Bytes (Hex): ");

            for (auto b : media) {
                // format compile-time :
                // :02X -> Hex UPCASE, width 2 charactor, auto add 0 for enough
                fmt::format_to(std::back_inserter(out), "{:02X} X ", b);
            }

            // HARIS_LOG_TRACE("HEX: {}", fmt::to_string(out));

            std::cout << fmt::to_string(out) << std::endl;
        }
    }

    void on_packet(const IPCRequestPayload& cmd) {
        HARIS_LOG_DEBUG("Data: [Command Payload] -> ID: {} | Cmd: {}", cmd.client_id, cmd.command);
    }
};

template <typename HandlerPolicy>
class PacketDispatcher {
 private:
    HandlerPolicy _handler;

 public:
    PacketDispatcher(const std::string& module) : _handler(module){};

    void dispatch(const PacketHeader& header, const std::vector<uint8_t>& payload) {
        switch (header.type) {
            case DataType::Heartbeat: {
                _handler.on_packet(IpcUtils::unpack<std::string>(payload));
                break;
            }
            case DataType::Text: {
                // cast at compile-time
                _handler.on_packet(IpcUtils::unpack<std::string>(payload));
                break;
            }
            case DataType::Number: {
                if (payload.size() == sizeof(double)) {
                    _handler.on_packet(IpcUtils::unpack<double>(payload));
                } else if (payload.size() == sizeof(int)) {
                    _handler.on_packet(IpcUtils::unpack<int>(payload));
                }
                break;
            }
            case DataType::Media: {
                _handler.on_packet(IpcUtils::unpack<std::vector<uint8_t>>(payload));
                break;
            }
            case DataType::Command: {
                _handler.on_packet(IpcUtils::unpack<IPCRequestPayload>(payload));
                break;
            }
            default:
                std::cerr << ">> Unknown DataType received in dispatcher pipeline.\n";
                break;
        }
    }
};

}  // namespace HarisLinux
