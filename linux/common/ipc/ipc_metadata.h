#pragma once
#include "std17pch.h"
#include "sfd/smart_file_descriptor.h"
#include "helper/fd_manager.h"

namespace HarisLinux {

enum class DataType : uint8_t { Number, Text, Command, Media, Heartbeat, Custom };

inline uint64_t get_current_timestamp_ms() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

struct __attribute__((packed)) PacketHeader {
    DataType type;          // 1 byte
    uint32_t payload_size;  // 4 bytes
    uint64_t timestamp_ms;  // 8 bytes // calc Hz
    uint32_t sequence_id;   // 4 bytes // check lose data
};

struct __attribute__((packed)) IPCRequestPayload {
    char     client_id[32];  // to naming for pipe
    uint32_t command;        // Command: 1 = Register, 2 = Disconnect)
};

namespace Ipc {
    enum class Role : uint8_t { None, Server, Client };

    enum class Server : uint8_t { ReadOnly = 0x00, Feedback = 0x01, CheckLose = 0x02, Broadcast = 0x04 };
    inline Server operator|(Server a, Server b) {
        return static_cast<Server>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    inline bool operator&(Server a, Server b) { return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0; }

    enum class Client : uint8_t { Send = 0x00, Feedback = 0x01, CheckLose = 0x02 };
    inline Client operator|(Client a, Client b) {
        return static_cast<Client>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }
    inline bool operator&(Client a, Client b) { return (static_cast<uint8_t>(a) & static_cast<uint8_t>(b)) != 0; }

    // 3. For both data type
    template <typename T>
    struct Generic {
        // Save data type
        static_assert(std::is_same_v<T, Server> || std::is_same_v<T, Client>,
                      "Ipc::Generic is a only once type: Server or Client!");

        T value;

        // compile time init data type
        Generic(T initial_value) : value(initial_value) {}

        // --- operator (Same data type) ---

        // operator OR (|) Valid: Return new Generic with same type
        Generic<T> operator|(T flag) const {
            return Generic<T>(static_cast<T>(static_cast<uint8_t>(value) | static_cast<uint8_t>(flag)));
        }

        // operator OR (|=) Valid: Return new Generic with same type
        Generic<T>& operator|=(T flag) {
            value = static_cast<T>(static_cast<uint8_t>(value) | static_cast<uint8_t>(flag));
            return *this;
        }

        // operator AND (&) Valid: Return new Generic with same type
        bool operator&(T flag) const { return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0; }

        // Add this inside struct Generic<T>
        bool has(T flag) const { return (*this & flag); }

        bool missing(T flag) const { return !has(flag); }

        template <typename U>
        Generic<T> operator|(U flag) const = delete;
        template <typename U>
        Generic<T>& operator|=(U flag) = delete;
        template <typename U>
        bool operator&(U flag) const = delete;
    };
}  // namespace Ipc

}  // namespace HarisLinux