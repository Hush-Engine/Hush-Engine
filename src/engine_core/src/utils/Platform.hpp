/*! \file Platform.hpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Platform detection macros
*/

#pragma once

#include <cstdint>

// NOLINTBEGIN(cppcoreguidelines-macro-usage)

#ifdef _WIN64
#define HUSH_PLATFORM_WIN 1
#define HUSH_PLATFORM_LINUX 0
#define HUSH_PLATFORM_OSX 0

#elif defined(__linux__) && !defined(__ANDROID__)
#define HUSH_PLATFORM_LINUX 1
#define HUSH_PLATFORM_WIN 0
#define HUSH_PLATFORM_OSX 0
#elif defined(__APPLE__) && defined(__MACH__)
#define HUSH_PLATFORM_OSX 1
#define HUSH_PLATFORM_LINUX 0
#define HUSH_PLATFORM_WIN 0
#else
#error "Platform not supported"
#endif


namespace Hush
{
    /// @brief Enum representing the current platform
    enum class EPlatform : uint8_t
    {
        Win64,
        Linux,
        OSX
    };

    /// @brief Get the current platform
    /// @return EPlatform
    inline constexpr EPlatform GetCurrentPlatform()
    {
#if HUSH_PLATFORM_WIN
        return EPlatform::Win64;
#elif HUSH_PLATFORM_LINUX
        return EPlatform::Linux;
#elif HUSH_PLATFORM_OSX
        return EPlatform::OSX;
#else
#error "Platform not supported"
#endif
    }
} // namespace Hush

// NOLINTEND(cppcoreguidelines-macro-usage)