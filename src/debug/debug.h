#pragma once

#include <iostream>
#include <sstream>
#include <string>

namespace debug {
#ifdef DEBUG_TOOLS
inline constexpr bool ToolsEnabled = true;
#else
inline constexpr bool ToolsEnabled = false;
#endif

bool isEnabled(const char *file);
const char *shortName(const char *file);
void print(std::ostream &out, const char *tag, const char *file, int line, const std::string &message);
}

#define LOG_WRITE(stream, tag, message) \
    do { \
        std::ostringstream logMessage; \
        logMessage << message; \
        debug::print(stream, tag, __FILE__, __LINE__, logMessage.str()); \
    } while (false)

#define LOG_INFO(message) \
    do { \
        if (debug::ToolsEnabled) { \
            LOG_WRITE(std::cout, "INFO ", message); \
        } \
    } while (false)
#define LOG_WARN(message)  LOG_WRITE(std::cerr, "WARN ", message)
#define LOG_ERROR(message) LOG_WRITE(std::cerr, "ERROR", message)

#ifdef NDEBUG
#define LOG_DEBUG(message) do { } while (false)
#else
#define LOG_DEBUG(message) \
    do { \
        if (debug::isEnabled(__FILE__)) { \
            LOG_WRITE(std::cout, "DEBUG", message); \
        } \
    } while (false)
#endif
