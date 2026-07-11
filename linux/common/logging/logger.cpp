#include "logger.h"

namespace HarisLinux {
// --- Logger Implementation ---
Logger::Logger(std::string name) : _name(std::move(name)), _level(LogLevel::Info) {}

void Logger::set_level(LogLevel level) { _level = level; }

std::string Logger::format_thread_process_info() {
    // 1. Get current time point from system clock
    auto now = std::chrono::system_clock::now();

    // 2. Convert to time_t for minutes/seconds decomposition
    auto    time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_struct;
    ::localtime_r(&time_t_now, &tm_struct);

    // 3. Extract milliseconds using fast duration casting (No complex divisions)
    auto duration     = now.time_since_epoch();
    auto seconds      = std::chrono::duration_cast<std::chrono::seconds>(duration);
    auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration - seconds).count();

    // 4. FAST ALGORITHM: Get last digit of minute without using operator '%'
    // (tm_min / 10) uses compiler-optimized multiplication shifts, then multiplied by 10 and subtracted.
    int min_last_digit = tm_struct.tm_min - ((tm_struct.tm_min / 10) * 10);

    // 5. Fetch both Process ID and current Thread ID from Linux OS
    pid_t pid, tid;
    RuntimeUtil::get_thread_and_process_id(pid, tid);
    char type_char = (pid == tid) ? 'P' : 'T';

    // Output pattern: T12224 4:48.125
    return fmt::format("{}{:<5} {:1d}:{:02d}.{:03d}", type_char, tid, min_last_digit, tm_struct.tm_sec,
                       static_cast<int>(milliseconds));
}

std::string_view Logger::format_level_to_string_view(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            return "TRACE ";
        case LogLevel::Debug:
            return "DEBUG ";
        case LogLevel::Info:
            return "INFO  ";
        case LogLevel::Warn:
            return "WARN  ";
        case LogLevel::Error:
            return "ERROR ";
        case LogLevel::Critical:
            return "CRIT  ";
        default:
            return " UNKNOWN";
    }
}

void Logger::std_cout_message(LogLevel level, std::string_view file, int line, std::string_view func,
                              const std::string& msg) {
    // Check if the input file view or function view is empty to prevent broken patterns
    if (file.empty()) file = "unknown_file";
    if (func.empty()) func = "unknown_func";

    // 1. Merge into a single compact pattern: file_clean:line Module:function
    // Optimization: std::string_view is formatted directly with zero temporary string allocation overhead
    std::string combined_metadata = fmt::format("{}:{} {}:{}", file, line, _name, func);

    // Pad the combined metadata block to 68 characters to ensure straight vertical alignment
    std::string padded_metadata = fmt::format("{:<68}", combined_metadata);

    // 2. Setup color formatting for level string
    std::string_view raw_level  = format_level_to_string_view(level);
    std::string_view color_code = "";
    std::string_view reset_code = "\033[0m";

    switch (level) {
        case LogLevel::Trace:
            color_code = "\033[37m";
            break;
        case LogLevel::Debug:
            color_code = "\033[36m";
            break;
        case LogLevel::Info:
            color_code = "\033[32m";
            break;
        case LogLevel::Warn:
            color_code = "\033[33m";
            break;
        case LogLevel::Error:
            color_code = "\033[31m";
            break;
        case LogLevel::Critical:
            color_code = "\033[1;35m";
            break;  // Bold Magenta
    }
    std::string formatted_level = fmt::format("{}{:<5}{}", color_code, raw_level, reset_code);

    // 3. Print out the structured log line
    std::cout << fmt::format("{} {} {} {}\n", format_thread_process_info(), formatted_level, padded_metadata, msg);
}

void Logger::std_cerr_message(std::string_view error_msg) {
    if (error_msg.empty()) return;
    std::cerr << "[LOG ERROR] Format failed: " << error_msg << "\n";
}

// --- LogRegistry Implementation ---
LogRegistry& LogRegistry::get_instance() {
    static LogRegistry instance;
    return instance;
}

std::shared_ptr<Logger> LogRegistry::get_logger(const std::string& name) {
    auto it = _loggers.find(name);
    if (it == _loggers.end()) {
        auto new_logger = std::make_shared<Logger>(name);
        _loggers[name]  = new_logger;
        return new_logger;
    }
    return it->second;
}

}  // namespace HarisLinux
