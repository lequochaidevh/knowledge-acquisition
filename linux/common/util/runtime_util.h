#pragma once

#include "std17pch.h"
#include "helper/nlohmann_json/json_helper.h"

namespace HarisLinux {
/**
 * @class RuntimeUtil
 * @brief Engine core providing zero-overhead, compile-time system utilities.
 * @details All methods are explicitly marked as static constexpr to ensure execution at build-time.
 */
class RuntimeUtil {
 public:
    // Delete constructor to prevent instantiation of a pure utility/static class
    RuntimeUtil() = delete;

    // ============================================================================
    // RUNTIME UTILITIES (New powerful additions)
    // ============================================================================

    /**
     * @brief Fetches both Process ID and Thread ID from Linux OS natively.
     * @param out_pid Reference to store the retrieved Process ID.
     * @param out_tid Reference to store the retrieved Thread ID.
     */
    static void get_thread_and_process_id(pid_t& out_pid, pid_t& out_tid) noexcept {
        out_pid = ::getpid();
        // Direct Linux syscall to get the accurate Kernel Thread ID (not pthread id)
        out_tid = static_cast<pid_t>(::syscall(SYS_gettid));
    }

    /**
     * @brief Decomposes system clock into exact time structures and sub-second parts.
     * @param out_tm Reference to store the calendar time structure (min, sec, etc.).
     * @param out_ms Reference to store the accurate millisecond remainder.
     */
    static void get_time(std::tm& out_tm, int& out_ms) noexcept {
        const auto now        = std::chrono::system_clock::now();
        const auto time_t_now = std::chrono::system_clock::to_time_t(now);

        // Thread-safe conversion to local time structure
        ::localtime_r(&time_t_now, &out_tm);

        const auto duration = now.time_since_epoch();
        const auto seconds  = std::chrono::duration_cast<std::chrono::seconds>(duration);

        // Extract isolated milliseconds without triggering modulo overhead
        out_ms = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(duration - seconds).count());
    }

    /**
     * @brief Retrieves the time element from the current system time.
     * @return uint64_t current time (ms).
     */
    static uint64_t get_current_time_ms() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())

            .count();
    }

    /**
     * @brief Retrieves the day element from the current system calendar time.
     * @return int The current day of the month (1-31).
     */
    static int get_day() noexcept {
        const auto now        = std::chrono::system_clock::now();
        const auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm    tm_struct;
        ::localtime_r(&time_t_now, &tm_struct);
        return tm_struct.tm_mday;
    }

    /**
     * @brief Generates a cryptographically-seeded pseudo-random word of length N.
     * @details Uses thread_local engine to eliminate race conditions and mutex locking costs.
     * @param length The total character length of the generated word.
     * @return std::string The randomized character string.
     */
    static std::string generate_random_word(size_t length) {
        if (length == 0) return "";

        // Standard alphanumeric character pool
        static constexpr std::string_view alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

        // Use thread_local to avoid contention overhead in multi-threaded systems
        thread_local std::mt19937             generator{std::random_device{}()};
        std::uniform_int_distribution<size_t> distribution(0, alphabet.size() - 1);

        std::string word;
        word.reserve(length);  // Optimization: Single allocation to avoid re-allocations

        for (size_t i = 0; i < length; ++i) {
            word += alphabet[distribution(generator)];
        }
        return word;
    }

    // ============================================================================
    // SCOPED SUBSYSTEM ALIASES (Zero-Overhead Composition Pattern)
    // ============================================================================

    /**
     * @brief Scoped accessor for all JSON configuration operations.
     * @note Prevents function name collisions with other file utility managers.
     */
    using Json = JsonHelper;
};

}  // namespace HarisLinux
