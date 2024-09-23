/*! \file Assertions.hpp
    \author Kyn21kx
    \date 2024-03-22
    \brief Macros to assert and provide debug breaks when a condition is not met
*/

#pragma once
#include "Platform.hpp"
#include "Logger.hpp"

#if HUSH_PLATFORM_WIN
#include <windows.h>
#if defined(_MSC_VER) || defined(__INTEL_COMPILER)
#define HUSH_DEBUG_BREAK __debugbreak()
#elif defined(__ARMCC_VERSION)
#define __breakpoint(42)
#endif
#else
#include <signal.h>
#ifdef SIGTRAP
#define HUSH_DEBUG_BREAK raise(SIGTRAP)
#else
#define HUSH_DEBUG_BREAK raise(SIGABRT)
#endif
#endif

// TODO: Add debug condition
// NOLINTNEXTLINE
#define HUSH_ASSERT(condition, fmtFormat, ...)                                                                         \
    if (!(condition))                                                                                                    \
    {                                                                                                                  \
        Hush::LogFormat(Hush::ELogLevel::Critical, "Assertion error! " fmtFormat, ##__VA_ARGS__);                      \
        HUSH_DEBUG_BREAK;                                                                                              \
    }

#define HUSH_STATIC_ASSERT(condition) static_assert(condition)