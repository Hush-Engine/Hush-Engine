/*! \file NetworkUtils.hpp
    \author Kyn21kx
    \date 2024-05-25
    \brief Provides simple utility functions for network/web related technologies
*/

#pragma once
#include "utils/Platform.hpp"
#include <string>
#include <utils/StringUtils.hpp>

namespace Hush::Networking
{

#if defined(HUSH_PLATFORM_WIN)
    template <uint32_t N> constexpr auto SystemOpenURL(const char (&url)[N])
    {
        const char *cmd = StringUtils::CompileTimeConcat("start ", url).data();
        return system(cmd);
    }
#elif defined(HUSH_PLATFORM_OSX)
    constexpr auto SystemOpenURL(const char (&url)[N])
    {
        const char *cmd = StringUtils::CompileTimeConcat("open ", url).data();
        return system(cmd);
    }
#elif defined(HUSH_PLATFORM_LINUX)
    constexpr auto SystemOpenURL(const char (&url)[N])
    {
        const char *cmd = StringUtils::CompileTimeConcat("xdg-open ", url).data();
        return system(cmd);
    }
#else
#error "Unknown compiler"
#endif
} // namespace Hush::Networking