#pragma once

#include "std17pch.h"

namespace HarisLinux {
/**
 * @class ConstexprUtil
 * @brief Engine core providing zero-overhead, compile-time system utilities.
 * @details All methods are explicitly marked as static constexpr to ensure execution at build-time.
 */
class ConstexprUtil {
 public:
    // Delete constructor to prevent instantiation of a pure utility/static class
    ConstexprUtil() = delete;

    /**
     * @brief Strips parent directory paths from a source file string.
     * @param filepath The raw path string from the compiler (usually via __FILE__).
     * @return std::string_view Pointing to the isolated filename.
     */
    static constexpr std::string_view trim_source_path(std::string_view filepath) noexcept {
        if (filepath.empty()) return "unknown";
        const size_t last_slash = filepath.find_last_of("/\\");
        return (last_slash != std::string_view::npos) ? filepath.substr(last_slash + 1) : filepath;
    }

    /**
     * @brief Generates a 64-bit FNV-1a hash from a string view at compile-time.
     * @param str The input string to be hashed.
     * @return uint64_t The calculated hash value.
     */
    static constexpr uint64_t const_hash(std::string_view str) noexcept {
        uint64_t hash = 0xcbf29ce484222325ULL;  // FNV-1a 64-bit offset basis
        for (const char c : str) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 0x100000001b3ULL;  // FNV-1a 64-bit prime
        }
        return hash;
    }

    /**
     * @brief Extracts the literal name of an enum value at compile-time.
     * @tparam E The Enum type.
     * @tparam V The specific Enum value.
     * @return std::string_view The clean name string of the enum value.
     */
    template <typename E, E V>
    static constexpr std::string_view get_enum_name() noexcept {
#if defined(__clang__) || defined(__GNUC__)
        std::string_view name  = __PRETTY_FUNCTION__;
        size_t           start = name.find_last_of("::");
        if (start == std::string_view::npos) start = name.find_last_of(' ');
#if defined(__clang__)
        size_t end = name.find(']');
#else  // GCC
        size_t end = name.find(';');
        if (end == std::string_view::npos) {
            end = name.find(']');
        }
#endif

        if (end != std::string_view::npos) {
            std::string_view sub   = name.substr(0, end);
            size_t           start = sub.find_last_of(": ");
            if (start != std::string_view::npos) {
                return sub.substr(start + 1);
            }
        }
        return "";
#else
        return "";
#endif
    }

    /**
     * @brief Resolves the exact type name of a variable or class at compile-time.
     * @tparam T The targeted data type.
     * @return std::string_view The human-readable type name.
     */
    template <typename T>
    static constexpr std::string_view get_type_name() noexcept {
#if defined(__clang__) || defined(__GNUC__)
        std::string_view name  = __PRETTY_FUNCTION__;
        size_t           start = name.find("T = ");
        if (start != std::string_view::npos) {
            start += 4;  // Advance past "T = "
            size_t end = name.find_first_of(";]", start);
            return name.substr(start, end - start);
        }
#endif
    }

    /**
     * @brief Extracts the clean class name from a function signature at compile-time.
     * @param func_sig The raw signature string from __PRETTY_FUNCTION__ or __FUNCSIG__.
     * @return A std::string_view pointing to the isolated class name.
     */
    /**
     * @brief Extracts the clean class name from a signature, stripping namespaces, methods, and templates at
     * compile-time.
     * @param func_sig The raw function signature string from __PRETTY_FUNCTION__ or __FUNCSIG__.
     * @return A std::string_view pointing only to the raw class name.
     */
    static constexpr std::string_view get_class_name(std::string_view func_sig) {
        // Step 1: Strip parameters by looking for the first open parenthesis '('
        size_t           paren        = func_sig.find('(');
        std::string_view dynamic_part = (paren != std::string_view::npos) ? func_sig.substr(0, paren) : func_sig;

        // Step 2: Find the actual class-scope "::" separator right before the function name
        size_t           last_space = dynamic_part.find_last_of(' ');
        std::string_view method_sig =
            (last_space != std::string_view::npos) ? dynamic_part.substr(last_space + 1) : dynamic_part;

        size_t scope_pos = method_sig.find_last_of(':');

        // If no "::" found or it's a malformed single colon, fallback to Global
        if (scope_pos == std::string_view::npos || scope_pos == 0 || method_sig[scope_pos - 1] != ':') {
            return "Global";
        }

        // Step 3: Class name with potential namespaces and templates is everything before "::"
        std::string_view class_part = method_sig.substr(0, scope_pos - 1);

        // Step 4: Extract only the class name if it resides inside nested namespaces
        size_t           last_ns_pos = class_part.find_last_of(':');
        std::string_view pure_class =
            (last_ns_pos != std::string_view::npos) ? class_part.substr(last_ns_pos + 1) : class_part;

        // Step 5: Strip template parameters (e.g., converts "PosixPipe<Modes>" to "PosixPipe")
        size_t template_pos = pure_class.find('<');
        if (template_pos != std::string_view::npos) {
            return pure_class.substr(0, template_pos);
        }

        return pure_class;
    }
};

// ============================================================================
// COMMANDER MACRO INTERFACES (Updated for Class syntax)
// ============================================================================

#define BUILD_LOG_LOCATION(msg)                                                \
    do {                                                                       \
        constexpr auto file_name = ConstexprUtil::trim_source_path(__FILE__);  \
        stdcout << "[" << file_name << ":" << __LINE__ << "] " << msg << "\n"; \
    } while (0)

#define CMD_PRINT_TYPE(variable)                                                             \
    do {                                                                                     \
        constexpr auto type_name = ConstexprUtil::get_type_name<decltype(variable)>();       \
        stdcout << "Variable '" << #variable << "' type resolved to: " << type_name << "\n"; \
    } while (0)

}  // namespace HarisLinux
