//
//  ScriptingManager.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 25/12/23.
//

#pragma once

#include <cstdint>
#include <fmt/format.h>
#include <string_view>

// NOLINTBEGIN(cppcoreguidelines-missing-std-forward)

namespace Hush
{
    // TODO: with C++20 we can use source_location to get the line where the message happened, should we raise the C++
    // version? https://en.cppreference.com/w/cpp/utility/source_location

    /// @brief Log level
    enum class ELogLevel : uint32_t
    {
        Trace,
        Debug,
        Info,
        Warn,
        Error,
        Critical
    };

    /// @brief Logs a message
    /// @param logLevel level to log the message
    /// @param message message to log
    void Log(ELogLevel logLevel, std::string_view message);

    /// @brief Logs a message with a given format
    template <class F, class... Args> void LogFormat(ELogLevel logLevel, F format, Args &&...args)
    {
        std::string message = fmt::format(format, std::forward<Args>(args)...);
        Log(logLevel, message);
    }

    /// @brief Logs a trace message
    /// @param message message to log
    inline void LogTrace(std::string_view message)
    {
        Log(ELogLevel::Trace, message);
    }

    /// @brief Logs a debug message
    /// @param message message to log
    inline void LogDebug(std::string_view message)
    {
        Log(ELogLevel::Debug, message);
    }

    /// @brief Logs an info message
    /// @param message message to log
    inline void LogInfo(std::string_view message)
    {
        Log(ELogLevel::Info, message);
    }

    /// @brief Logs a warning message
    /// @param message message to log
    inline void LogWarn(std::string_view message)
    {
        Log(ELogLevel::Warn, message);
    }

    /// @brief Logs an error message
    /// @param message message to log
    inline void LogError(std::string_view message)
    {
        Log(ELogLevel::Error, message);
    }

    /// @brief Logs a critical message
    /// @param message message to log
    inline void LogCritical(std::string_view message)
    {
        Log(ELogLevel::Critical, message);
    }

    inline void ClearLogs()
    {
#if defined(WIN32)
        system("cls");
#else
        system("clear");
#endif
    }
} // namespace Hush

// NOLINTEND(cppcoreguidelines-missing-std-forward)
