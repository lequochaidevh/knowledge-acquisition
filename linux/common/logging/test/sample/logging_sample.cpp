#include <iostream>
#include <string>
#include <string_view>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fmt/core.h>  // Required: Needs fmtlib installed (header-only or compiled)

// Log levels enumeration
enum class LogLevel { Trace, Debug, Info, Warn, Error, Critical };

class Logger {
 public:
    // Get singleton instance
    static Logger& getInstance() {
        static Logger instance;
        return instance;
    }

    // Deleted copy constructor and assignment operator for singleton
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    // Set global log level
    void setLevel(LogLevel level) { _level = level; }

    // Core log function using C++17 and fmt::format
    template <typename... Args>
    void log(LogLevel level, std::string_view fmt, Args&&... args) {
        if (level < _level) return;

        try {
            // Format the user message using fmtlib (C++17 compatible)
            std::string user_msg = fmt::format(fmt, std::forward<Args>(args)...);

            // Print formatted log with timestamp and colors
            std::cout << fmt::format("[{}] [{}] {}\n", getTimestamp(), levelToString(level), user_msg);
        }
        // Catch generic std::exception to bypass any macro naming conflicts with format_error
        catch (const std::exception& e) {
            std::cerr << "[LOG ERROR] Format failed: " << e.what() << "\n";
        }
    }

 private:
    // Private constructor
    Logger() : _level(LogLevel::Info) {}

    // Member variables with private naming convention
    LogLevel _level;

    // Get current time in [YYYY-MM-DD HH:MM:SS] format using C++17 features
    std::string getTimestamp() {
        auto    now        = std::chrono::system_clock::now();
        auto    time_t_now = std::chrono::system_clock::to_time_t(now);
        std::tm tm_struct;

        // Thread-safe alternative to std::localtime
        localtime_r(&time_t_now, &tm_struct);

        std::stringstream ss;
        ss << std::put_time(&tm_struct, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // Convert level to colored string using ANSI escape codes
    std::string_view levelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace:
                return "\033[37mTRACE\033[0m";  // White
            case LogLevel::Debug:
                return "\033[36mDEBUG\033[0m";  // Cyan
            case LogLevel::Info:
                return "\033[32mINFO\033[0m";  // Green
            case LogLevel::Warn:
                return "\033[33mWARN\033[0m";  // Yellow
            case LogLevel::Error:
                return "\033[31mERROR\033[0m";  // Red
            case LogLevel::Critical:
                return "\033[1;31mCRITICAL\033[0m";  // Bold Red
            default:
                return "UNKNOWN";
        }
    }
};

// Convenient macro definitions mimicking spdlog syntax
#define LOG_TRACE(fmt, ...)    Logger::getInstance().log(LogLevel::Trace, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)    Logger::getInstance().log(LogLevel::Debug, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)     Logger::getInstance().log(LogLevel::Info, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)     Logger::getInstance().log(LogLevel::Warn, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)    Logger::getInstance().log(LogLevel::Error, fmt, ##__VA_ARGS__)
#define LOG_CRITICAL(fmt, ...) Logger::getInstance().log(LogLevel::Critical, fmt, ##__VA_ARGS__)

int main() {
    // Configure logger to show all logs from Debug level and above
    Logger::getInstance().setLevel(LogLevel::Debug);

    // Test different log levels with standard C++17 {} formatting via fmtlib
    LOG_DEBUG("Connecting to IP: {} on Port: {}", "119.2.1.5", 8080);
    LOG_INFO("System initialized successfully. Status code: {}", 200);
    LOG_WARN("Battery low: {}% remaining!", 15.5);
    LOG_ERROR("Failed to write to file: {}", "/var/log/sys.log");
    LOG_CRITICAL("Hardware failure detected! Emergency shutdown initiated.");

    // This will not be printed because global level is set to Debug
    LOG_TRACE("This is a trace message");

    return 0;
}