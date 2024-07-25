#include <print>

import logging;

using namespace wjgra;

int main() {
    loggingLevel = LOGGING_LEVEL_WARNING;
    logError("log message... {}, {}, {}", 1, "two", 3);
    logWarning("log message... {}, {}, {}", 1, "two", 3);
    logInfo("log message... {}, {}, {}", 1, "two", 3);
    logDebug("log message... {}, {}, {}", 1, "two", 3);
    return 0;
}