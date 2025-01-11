#ifndef _UTIL_LOGGING_HPP_
#define _UTIL_LOGGING_HPP_

#include <algorithm>
#include <array>
#include <print>
#include <ranges>
#include <string>
#include <string_view>

namespace util {

enum LoggingLevel : uint16_t {
    LOGGING_LEVEL_NONE = 0,
    LOGGING_LEVEL_ERROR,
    LOGGING_LEVEL_WARNING,
    LOGGING_LEVEL_INFO,
    LOGGING_LEVEL_DEBUG
};

struct Logger {
    static auto getLoggingLevel() noexcept { return loggingLevel; }

    static void setLoggingLevel(const LoggingLevel newLevel)
    {
        if (newLevel >= labels.size()) {
            throw std::invalid_argument("logging level out of range");
        }
        loggingLevel = newLevel;
    }

    static auto loggingLevelStr(const LoggingLevel level = loggingLevel) noexcept { return labels[level]; }

    template <class... Args>
    static void logMessage(const LoggingLevel level, std::format_string<Args...> message, Args&&... args) noexcept
    {
        if (loggingLevel >= level) {
            // Print label at fixed width, followed by formatted message
            std::println(
                "[{:^{}}]: {}", labels[level], len_longestLabel, std::format(message, std::forward<Args>(args)...));
        }
    }

    // Get a help string listing the different logging levels
    static constexpr auto helpText()
    {
        std::string help_str = "set logging level (";
        for (size_t log_idx = 0; log_idx < labels.size(); ++log_idx) {
            static_assert(labels.size() <= 10); // Verify log level fits into one character
            help_str += std::string(1, '0' + log_idx);
            help_str += " = ";
            help_str += labels[log_idx];
            if (log_idx != labels.size() - 1) {
                help_str += ", ";
            }
        }
        help_str += ")";
        return help_str
    }

private:
    static inline LoggingLevel loggingLevel{LOGGING_LEVEL_INFO};
    static constexpr auto labels = std::to_array<std::string_view>({"NONE", "ERROR", "WARNING", "INFO", "DEBUG"});

    // Length of the longest label is used to print log labels with a fixed width
    static constexpr auto len_longestLabel =
        std::ranges::max(labels | std::views::transform([](auto&& str) { return str.size(); }));
};

// Log functions for specific logging levels:

template <class... Args>
void logError(std::format_string<Args...> message, Args&&... args) noexcept
{
    Logger::logMessage(LOGGING_LEVEL_ERROR, message, std::forward<Args>(args)...);
}

template <class... Args>
void logWarning(std::format_string<Args...> message, Args&&... args) noexcept
{
    Logger::logMessage(LOGGING_LEVEL_WARNING, message, std::forward<Args>(args)...);
}

template <class... Args>
void logInfo(std::format_string<Args...> message, Args&&... args) noexcept
{
    Logger::logMessage(LOGGING_LEVEL_INFO, message, std::forward<Args>(args)...);
}

template <class... Args>
void logDebug(std::format_string<Args...> message, Args&&... args) noexcept
{
    Logger::logMessage(LOGGING_LEVEL_DEBUG, message, std::forward<Args>(args)...);
}

} // namespace util

#endif