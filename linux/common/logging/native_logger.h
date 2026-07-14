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
    NativeLogStream(std::mutex& mutex, const std::string& color) : _stream_mutex(mutex) {
        _buffer << color;  // Append color code at the beginning
    }

    // Destructor appends \n, flushes to console under lock, and resets color
    ~NativeLogStream() {
        _buffer << LOG_COLOR_RESET << "\n";
        std::lock_guard<std::mutex> lock(_stream_mutex);
        std::cout << _buffer.str();
    }

    // Support standard data types via operator<<
    template <typename T>
    NativeLogStream& operator<<(const T& value) {
        _buffer << value;
        return *this;
    }

    // Intercept and ignore stream manipulators like std::endl
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

    // Thread-safe Singleton instance accessor (C++11 Meyer's Singleton)
    static NativeLogger& get_instance() {
        static NativeLogger _instance;
        return _instance;
    }

    // Creates a new log stream with a specific color
    NativeLogStream create_stream(const std::string& color) { return NativeLogStream(_log_mutex, color); }

    // Core recursive function to handle basic format printing ("{}")
    void format_print(std::ostringstream& oss, const std::string& fmt) { oss << fmt; }

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

    // Public format logging function that handles thread safety and coloring
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
#define stdcout NativeLogger::get_instance().create_stream(LOG_COLOR_WARN)

// Macro helpers for streaming log levels
#define native_log_info  NativeLogger::get_instance().create_stream(LOG_COLOR_INFO)
#define native_log_warn  NativeLogger::get_instance().create_stream(LOG_COLOR_WARN)
#define native_log_error NativeLogger::get_instance().create_stream(LOG_COLOR_ERROR)

// Macro helpers for formatted log levels
#define native_print_info(fmt, ...)  NativeLogger::get_instance().log_fmt(LOG_COLOR_INFO, fmt, ##__VA_ARGS__)
#define native_print_warn(fmt, ...)  NativeLogger::get_instance().log_fmt(LOG_COLOR_WARN, fmt, ##__VA_ARGS__)
#define native_print_error(fmt, ...) NativeLogger::get_instance().log_fmt(LOG_COLOR_ERROR, fmt, ##__VA_ARGS__)

}  // namespace HarisLinux
