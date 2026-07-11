#pragma once

#include <fmt/core.h>
#include <fmt/format.h>

#include "util/command_util.h"

namespace HarisLinux {

enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical };

class Logger {
 public:
    // Constructor requires a name for the module logger
    explicit Logger(std::string name);

    // Set level for this specific logger instance
    void set_level(LogLevel level);

    LogLevel get_level() const { return _level; };

    // Format and print log if level matches
    template <typename... Args>
    void log(LogLevel level, std::string_view file, int line, std::string_view func, std::string_view fmt,
             Args&&... args) {
        // Early exit without evaluating arguments if the log level is filtered out
        if (level < _level) return;

        try {
            // Construct the custom user log message using type-safe formatting
            std::string user_msg = fmt::format(fmt, std::forward<Args>(args)...);

            // Pass the optimized string_views directly to the output layer
            std_cout_message(level, file, line, func, user_msg);
        } catch (const std::exception& e) {
            // Fallback safety path if string formatting encounters a runtime error
            std_cerr_message(e.what());
        }
    }

 private:
    std::string _name;
    LogLevel    _level;

    std::string      format_thread_process_info();
    std::string_view format_level_to_string_view(LogLevel level);
    void             std_cout_message(LogLevel level, std::string_view file, int line, std::string_view func,
                                      const std::string& msg);
    void             std_cerr_message(std::string_view error_msg);
};

// Global Registry to manage and look up loggers by name across modules
class LogRegistry {
 public:
    static LogRegistry& get_instance();

    LogRegistry(const LogRegistry&) = delete;
    LogRegistry& operator=(const LogRegistry&) = delete;

    // Create or get an existing logger for a module
    std::shared_ptr<Logger> get_logger(const std::string& name);

 private:
    LogRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<Logger>> _loggers;
};

// Macros designed to use a local or module-specific logger variable named 'logger'

#define DECLARE_LOGGER inline static std::shared_ptr<Logger> logger
#define INIT_LOGGER(module)                                      \
    if (!logger) {                                               \
        logger = LogRegistry::get_instance().get_logger(module); \
    }
// ============================================================================
// HIGH-PERFORMANCE COMPILE-TIME LOGGING MACROS
// ============================================================================

#define HARIS_LOG_TRACE(fmt, ...)                                                                     \
    do {                                                                                              \
        constexpr std::string_view static_file = ConstexprUtil::trim_source_path(__FILE__);           \
        logger->log(LogLevel::Trace, static_file.data(), __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
    } while (0)

#define HARIS_LOG_DEBUG(fmt, ...)                                                                     \
    do {                                                                                              \
        constexpr std::string_view static_file = ConstexprUtil::trim_source_path(__FILE__);           \
        logger->log(LogLevel::Debug, static_file.data(), __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
    } while (0)

#define HARIS_LOG_INFO(fmt, ...)                                                                     \
    do {                                                                                             \
        constexpr std::string_view static_file = ConstexprUtil::trim_source_path(__FILE__);          \
        logger->log(LogLevel::Info, static_file.data(), __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
    } while (0)

#define HARIS_LOG_WARN(fmt, ...)                                                                     \
    do {                                                                                             \
        constexpr std::string_view static_file = ConstexprUtil::trim_source_path(__FILE__);          \
        logger->log(LogLevel::Warn, static_file.data(), __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
    } while (0)

#define HARIS_LOG_ERROR(fmt, ...)                                                                     \
    do {                                                                                              \
        constexpr std::string_view static_file = ConstexprUtil::trim_source_path(__FILE__);           \
        logger->log(LogLevel::Error, static_file.data(), __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
    } while (0)

#define HARIS_LOG_CRITICAL(fmt, ...)                                                                     \
    do {                                                                                                 \
        constexpr std::string_view static_file = ConstexprUtil::trim_source_path(__FILE__);              \
        logger->log(LogLevel::Critical, static_file.data(), __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__); \
    } while (0)

}  // namespace HarisLinux
