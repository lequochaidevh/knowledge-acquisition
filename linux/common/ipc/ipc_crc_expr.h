
#include "std17pch.h"

namespace HarisLinux {

/**
 * @brief Core template traits registry managing compile-time validation profiles for type variations.
 * @tparam T The targeted payload payload object structure template type.
 */
template <typename T>
struct StaticPacketTraits {
    static constexpr bool     is_static  = false;
    static constexpr const T* static_ptr = nullptr;
    static constexpr uint32_t crc32      = 0;
};

/**
 * @brief Macro targeting explicit template profile specification injection into the HarisLinux namespace.
 * Automatically resolves structural token content loops into static constants at compile-time.
 */
#define REGISTER_STATIC_PACKET(TypeStruct, StaticInstance)                                                        \
    namespace HarisLinux {                                                                                        \
        template <>                                                                                               \
        struct StaticPacketTraits<TypeStruct> {                                                                   \
            static constexpr bool              is_static  = true;                                                 \
            static constexpr const TypeStruct* static_ptr = &StaticInstance;                                      \
            static constexpr uint32_t          crc32      = ConstexprUtil::calculate_crc32(StaticInstance.token); \
        };                                                                                                        \
    }

/**
 * @brief Light structure allocating pointer index tracking to bypass continuous dynamic string verification.
 */
struct RuntimeStringCache {
    const uint8_t* last_seen_ptr = nullptr;  // Tracks raw pointer address to the continuous buffer content (.data())
    uint32_t       cached_crc    = 0;        // Keeps the matching computed checksum cached for O(1) recall
};

}  // namespace HarisLinux
