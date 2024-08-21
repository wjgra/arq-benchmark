#ifndef _UTIL_LOGGING_HPP_
#define _UTIL_LOGGING_HPP_

#include <algorithm>
#include <array>
#include <print>
#include <ranges>
#include <string_view>
#include <string>

namespace util {

enum LoggingLevel {
    LOGGING_LEVEL_NONE = 0,
    LOGGING_LEVEL_ERROR,
    LOGGING_LEVEL_WARNING,
    LOGGING_LEVEL_INFO,
    LOGGING_LEVEL_DEBUG
};

struct Logger {
    static auto getLoggingLevel() {
        return loggingLevel;
    }

    static void setLoggingLevel(const LoggingLevel newLevel) {
        if (!(newLevel >= 0 && newLevel < labels.size())) {
            throw std::invalid_argument("logging level out of range");
        }
        loggingLevel = newLevel;
    }

    static auto loggingLevelStr(const LoggingLevel level = loggingLevel) {
        return labels[level];
    }

    template <class... Args>
    static void logMessage(const LoggingLevel level, std::format_string<Args...> message, Args&&... args) {
        if (loggingLevel >= level) {
            // Print label at fixed width, followed by formatted message
            std::println("[{:^{}}]: {}",
                        labels[level],
                        len_longestLabel,
                        std::format(message, std::forward<Args>(args)...));
        }
    }

    static auto helpText() {
        std::string out = "set logging level (";
        for (size_t i = 0 ; i < labels.size() ; ++i) {
            out += std::to_string(i) + " = " + std::string{labels[i]};
            if (i != labels.size() - 1) {
                out += ", ";
            }
        }
        return (out += ")");
    }

private:
    static inline LoggingLevel loggingLevel{LOGGING_LEVEL_INFO};
    static constexpr auto labels = std::to_array<std::string_view>({
        "NONE",
        "ERROR",
        "WARNING",
        "INFO",
        "DEBUG"
    });
    // Length of the longest label used to print log labels with a fixed with, ensuring that messages align
    static constexpr auto len_longestLabel = std::ranges::max(labels | std::views::transform([](auto&& str){ return str.size(); }));
};

// Log functions for specific logging levels:

template <class... Args>
void logError(std::format_string<Args...> message, Args&&... args) {
    Logger::logMessage(LOGGING_LEVEL_ERROR, message, std::forward<Args>(args)...);
}

template <class... Args>
void logWarning(std::format_string<Args...> message, Args&&... args) {
    Logger::logMessage(LOGGING_LEVEL_WARNING, message, std::forward<Args>(args)...);
}

template <class... Args>
void logInfo(std::format_string<Args...> message, Args&&... args) {
    Logger::logMessage(LOGGING_LEVEL_INFO, message, std::forward<Args>(args)...);
}

template <class... Args>
void logDebug(std::format_string<Args...> message, Args&&... args) {
    Logger::logMessage(LOGGING_LEVEL_DEBUG, message, std::forward<Args>(args)...);
}

}

#endif