#pragma once

#include "std17pch.h"

namespace HarisLinux {
// ANSI color codes for console output
#define LOG_COLOR_RESET "\033[0m"
#define LOG_COLOR_INFO  "\033[32m"  // Green
#define LOG_COLOR_WARN  "\033[33m"  // Yellow
#define LOG_COLOR_ERROR "\033[31m"  // Red

// Helper class to accumulate log data using the << operator
class NativeLogStream {
 public:
    /**
     * @brief Constructor that initializes the log stream with a specific color.
     * @param mutex Reference to the parent logger's mutex for synchronization.
     * @param color ANSI color code string to apply at the start of the log line.
     */
    NativeLogStream(std::mutex& mutex, const std::string& color) : _stream_mutex(mutex) {
        _buffer << color;  // Append color code at the beginning
    }

    /**
     * @brief Destructor that appends a newline, resets the color, and flushes to stdout.
     * @note This operation is thread-safe as it locks the shared stream mutex.
     */
    ~NativeLogStream() {
        _buffer << LOG_COLOR_RESET << "\n";
        std::lock_guard<std::mutex> lock(_stream_mutex);
        std::cout << _buffer.str();
    }

    /**
     * @brief Stream insertion operator to append standard data types to the log buffer.
     * @tparam T Any type that supports the standard ostream insertion operator.
     * @param value The data to be logged.
     * @return Reference to the current NativeLogStream object for chaining.
     */
    template <typename T>
    NativeLogStream& operator<<(const T& value) {
        _buffer << value;
        return *this;
    }

    /**
     * @brief Interceptor for stream manipulators to completely ignore std::endl.
     * @param manip Pointer to the stream manipulator function (e.g., std::endl).
     * @return Reference to the current NativeLogStream object to continue chaining.
     */
    NativeLogStream& operator<<(std::ostream& (*manip)(std::ostream&)) { return *this; }

 private:
    std::ostringstream _buffer;
    std::mutex&        _stream_mutex;
};

// Main Singleton class for native console logging
class NativeLogger {
 public:
    // Delete copy constructor and assignment operator for Singleton pattern
    NativeLogger(const NativeLogger&) = delete;
    NativeLogger& operator=(const NativeLogger&) = delete;

    /**
     * @brief Retrieves the single global instance of the logger.
     * @note Implements Meyer's Singleton which guarantees thread-safe initialization since C++11.
     * @return Reference to the unique NativeConsoleLogger instance.
     */
    static NativeLogger& get_instance() {
        static NativeLogger _instance;
        return _instance;
    }

    /**
     * @brief Factory method to spin up a temporary stream object for a specific color/level.
     * @param color The ANSI color code to format the stream output.
     * @return A temporary NativeLogStream object instance.
     */
    NativeLogStream create_stream(const std::string& color) { return NativeLogStream(_log_mutex, color); }

    /**
     * @brief Base case for the recursive variadic print function to append trailing text.
     * @param oss The output string stream collecting the final formatted message.
     * @param fmt The remaining format string with no placeholders left to process.
     */
    void format_print(std::ostringstream& oss, const std::string& fmt) { oss << fmt; }

    /**
     * @brief Recursive variadic function to replace "{}" placeholders with actual values.
     * @tparam T Type of the current argument being processed.
     * @tparam Args Types of the remaining arguments.
     * @param oss The output string stream capturing the final message data.
     * @param fmt The format string containing text and "{}" placeholders.
     * @param first The current argument value to plug into the placeholder.
     * @param args The remaining arguments waiting to be processed.
     */
    template <typename T, typename... Args>
    void format_print(std::ostringstream& oss, const std::string& fmt, const T& first, const Args&... args) {
        size_t pos = fmt.find("{}");
        if (pos == std::string::npos) {
            oss << fmt;
            return;
        }
        oss << fmt.substr(0, pos) << first;
        format_print(oss, fmt.substr(pos + 2), args...);
    }

    /**
     * @brief High-level API to print thread-safe, colored, and formatted log lines.
     * @tparam Args Types of the arguments to inject into the placeholders.
     * @param color The ANSI color code for this specific log message.
     * @param fmt The log format template string with "{}" placeholders.
     * @param args The dynamic arguments to format into the template.
     */
    template <typename... Args>
    void log_fmt(const std::string& color, const std::string& fmt, const Args&... args) {
        std::ostringstream oss;
        oss << color;
        format_print(oss, fmt, args...);
        oss << LOG_COLOR_RESET << "\n";

        std::lock_guard<std::mutex> lock(_log_mutex);
        std::cout << oss.str();
    }

 private:
    NativeLogger() = default;  // Private constructor
    std::mutex _log_mutex;
};

// Macro replacement for std::cout using the stream approach
#define stdinfo NativeLogger::get_instance().create_stream(LOG_COLOR_INFO)
#define stdcout NativeLogger::get_instance().create_stream(LOG_COLOR_WARN)
#define stdcerr NativeLogger::get_instance().create_stream(LOG_COLOR_ERROR)

// Macro helpers for streaming log levels
#define native_log_info  NativeLogger::get_instance().create_stream(LOG_COLOR_INFO)
#define native_log_warn  NativeLogger::get_instance().create_stream(LOG_COLOR_WARN)
#define native_log_error NativeLogger::get_instance().create_stream(LOG_COLOR_ERROR)

// Macro helpers for formatted log levels
#define native_print_info(fmt, ...)  NativeLogger::get_instance().log_fmt(LOG_COLOR_INFO, fmt, ##__VA_ARGS__)
#define native_print_warn(fmt, ...)  NativeLogger::get_instance().log_fmt(LOG_COLOR_WARN, fmt, ##__VA_ARGS__)
#define native_print_error(fmt, ...) NativeLogger::get_instance().log_fmt(LOG_COLOR_ERROR, fmt, ##__VA_ARGS__)

}  // namespace HarisLinux
