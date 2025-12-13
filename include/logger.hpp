// Logging wrapper using spdlog
// Provides convenient logging interface for the simulator

#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/fmt/fmt.h>
#include <memory>
#include <string>

namespace logger {

// Initialize logger (call once at program start)
inline void init() {
    auto console = spdlog::stdout_color_mt("console");
    spdlog::set_default_logger(console);
    spdlog::set_pattern("[%H:%M:%S.%e] [%^%l%$] %v");
    spdlog::set_level(spdlog::level::info);
}

// Set log level
inline void set_level(spdlog::level::level_enum level) {
    spdlog::set_level(level);
}

// Logging functions using fmt::format
template<typename... Args>
inline void trace(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::trace(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void debug(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::debug(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void info(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::info(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void warn(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::warn(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void error(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::error(fmt, std::forward<Args>(args)...);
}

template<typename... Args>
inline void critical(fmt::format_string<Args...> fmt, Args&&... args) {
    spdlog::critical(fmt, std::forward<Args>(args)...);
}

} // namespace logger
