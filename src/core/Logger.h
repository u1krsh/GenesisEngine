#pragma once

#include <string>
#include <iostream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <functional>

namespace Genesis {

// ============================================================================
// Log Levels
// ============================================================================
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

// ============================================================================
// Logger - Simple logging system with console integration
// ============================================================================
class Logger {
public:
    static Logger& Instance() {
        static Logger instance;
        return instance;
    }

    void SetMinLevel(LogLevel level) { m_minLevel = level; }
    LogLevel GetMinLevel() const { return m_minLevel; }

    // Set callback to forward logs to console
    using ConsoleCallback = std::function<void(LogLevel, const std::string&, const std::string&)>;
    void SetConsoleCallback(ConsoleCallback callback) { m_consoleCallback = callback; }

    void Log(LogLevel level, const std::string& category, const std::string& message) {
        if (level < m_minLevel) return;

        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;

        std::ostringstream oss;
        oss << std::put_time(std::localtime(&time), "%H:%M:%S");
        oss << '.' << std::setfill('0') << std::setw(3) << ms.count();
        oss << " [" << LevelToString(level) << "] ";
        oss << "[" << category << "] ";
        oss << message;

        // Color output based on level
        switch (level) {
            case LogLevel::Trace:   std::cout << "\033[90m"; break;  // Gray
            case LogLevel::Debug:   std::cout << "\033[36m"; break;  // Cyan
            case LogLevel::Info:    std::cout << "\033[32m"; break;  // Green
            case LogLevel::Warning: std::cout << "\033[33m"; break;  // Yellow
            case LogLevel::Error:   std::cout << "\033[31m"; break;  // Red
            case LogLevel::Fatal:   std::cout << "\033[35m"; break;  // Magenta
        }
        std::cout << oss.str() << "\033[0m" << std::endl;

        // Forward to console if callback is set
        if (m_consoleCallback) {
            m_consoleCallback(level, category, message);
        }
    }

private:
    Logger() = default;

    const char* LevelToString(LogLevel level) {
        switch (level) {
            case LogLevel::Trace:   return "TRACE";
            case LogLevel::Debug:   return "DEBUG";
            case LogLevel::Info:    return "INFO ";
            case LogLevel::Warning: return "WARN ";
            case LogLevel::Error:   return "ERROR";
            case LogLevel::Fatal:   return "FATAL";
            default:                return "?????";
        }
    }

    LogLevel m_minLevel = LogLevel::Info;
    ConsoleCallback m_consoleCallback = nullptr;
};

// ============================================================================
// Logging Macros
// ============================================================================
#define LOG_TRACE(category, msg)   Genesis::Logger::Instance().Log(Genesis::LogLevel::Trace, category, msg)
#define LOG_DEBUG(category, msg)   Genesis::Logger::Instance().Log(Genesis::LogLevel::Debug, category, msg)
#define LOG_INFO(category, msg)    Genesis::Logger::Instance().Log(Genesis::LogLevel::Info, category, msg)
#define LOG_WARNING(category, msg) Genesis::Logger::Instance().Log(Genesis::LogLevel::Warning, category, msg)
#define LOG_ERROR(category, msg)   Genesis::Logger::Instance().Log(Genesis::LogLevel::Error, category, msg)
#define LOG_FATAL(category, msg)   Genesis::Logger::Instance().Log(Genesis::LogLevel::Fatal, category, msg)

} // namespace Genesis

