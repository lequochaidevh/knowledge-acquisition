#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <fmt/core.h>

enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical };

class Logger {
 public:
    // Constructor requires a name for the module logger
    explicit Logger(std::string name);

    // Set level for this specific logger instance
    void setLevel(LogLevel level);

    // Format and print log if level matches
    template <typename... Args>
    void log(LogLevel level, std::string_view fmt, Args&&... args) {
        if (level < _level) return;

        try {
            std::string user_msg = fmt::format(fmt, std::forward<Args>(args)...);
            printLog(level, user_msg);
        } catch (const std::exception& e) {
            printError(e.what());
        }
    }

 private:
    std::string _name;
    LogLevel    _level;

    std::string      getThreadProcessInfo();
    std::string_view levelToString(LogLevel level);
    void             printLog(LogLevel level, const std::string& msg);
    void             printError(const char* error_msg);
};

// Global Registry to manage and look up loggers by name across modules
class LogRegistry {
 public:
    static LogRegistry& getInstance();

    LogRegistry(const LogRegistry&) = delete;
    LogRegistry& operator=(const LogRegistry&) = delete;

    // Create or get an existing logger for a module
    std::shared_ptr<Logger> getLogger(const std::string& name);

 private:
    LogRegistry() = default;
    std::unordered_map<std::string, std::shared_ptr<Logger>> _loggers;
};

// Macros designed to use a local or module-specific logger variable named 'logger'
#define HARIS_LOG_TRACE(fmt, ...)    logger->log(LogLevel::Trace, fmt, ##__VA_ARGS__)
#define HARIS_LOG_DEBUG(fmt, ...)    logger->log(LogLevel::Debug, fmt, ##__VA_ARGS__)
#define HARIS_LOG_INFO(fmt, ...)     logger->log(LogLevel::Info, fmt, ##__VA_ARGS__)
#define HARIS_LOG_WARN(fmt, ...)     logger->log(LogLevel::Warn, fmt, ##__VA_ARGS__)
#define HARIS_LOG_ERROR(fmt, ...)    logger->log(LogLevel::Error, fmt, ##__VA_ARGS__)
#define HARIS_LOG_CRITICAL(fmt, ...) logger->log(LogLevel::Critical, fmt, ##__VA_ARGS__)
