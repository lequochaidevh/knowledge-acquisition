#include "logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

// --- Logger Implementation ---
Logger::Logger(std::string name) : _name(std::move(name)), _level(LogLevel::Info) {}

void Logger::setLevel(LogLevel level) { _level = level; }

std::string Logger::getTimestamp() {
    auto    now        = std::chrono::system_clock::now();
    auto    time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_struct;

    localtime_r(&time_t_now, &tm_struct);

    std::stringstream ss;
    ss << std::put_time(&tm_struct, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

std::string_view Logger::levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::Trace:
            return "\033[37mTRACE\033[0m";
        case LogLevel::Debug:
            return "\033[36mDEBUG\033[0m";
        case LogLevel::Info:
            return "\033[32mINFO\033[0m";
        case LogLevel::Warn:
            return "\033[33mWARN\033[0m";
        case LogLevel::Error:
            return "\033[31mERROR\033[0m";
        case LogLevel::Critical:
            return "\033[1;31mCRITICAL\033[0m";
        default:
            return "UNKNOWN";
    }
}

void Logger::printLog(LogLevel level, const std::string& msg) {
    // Modified format to display the specific module name: [TIMESTAMP] [MODULE_NAME] [LEVEL] MESSAGE
    std::cout << fmt::format("[{}] [{}] [{}] {}\n", getTimestamp(), _name, levelToString(level), msg);
}

void Logger::printError(const char* error_msg) { std::cerr << "[LOG ERROR] Format failed: " << error_msg << "\n"; }

// --- LogRegistry Implementation ---
LogRegistry& LogRegistry::getInstance() {
    static LogRegistry instance;
    return instance;
}

std::shared_ptr<Logger> LogRegistry::getLogger(const std::string& name) {
    auto it = _loggers.find(name);
    if (it == _loggers.end()) {
        auto new_logger = std::make_shared<Logger>(name);
        _loggers[name]  = new_logger;
        return new_logger;
    }
    return it->second;
}
