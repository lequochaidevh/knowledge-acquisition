#pragma once
#include <cstdint>
#include <cstring>

class ByteUtilities {
 public:
    // Safe binary copy function that abstracts memory operations
    template <typename T>
    static void serialize_type(uint8_t* destination, const T& value) {
        std::memcpy(destination, &value, sizeof(T));
    }

    // Emulates a very basic Fletcher16 or custom checksum evaluation utility
    static uint16_t calculate_checksum(const uint8_t* data, size_t size) {
        uint16_t sum = 0;
        for (size_t i = 0; i < size; ++i) {
            sum += data[i];
        }
        return sum;
    }
};
