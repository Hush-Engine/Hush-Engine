/*! \file Assertions.hpp
	\author Kyn21kx
	\date 2024-03-22
	\brief Macros to assert and provide debug breaks when a condition is not met
*/

#pragma once
#include "Platform.hpp"
#include "magic_enum/magic_enum.hpp"
#include "Logger.hpp"

#if HUSH_PLATFORM_WIN
#include <windows.h>
// windows.h is cancer, let's hope for a slimer implementation in the future
#undef min
#undef max
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
#if defined(DEBUG) && !defined(NO_ASSERT)
// NOLINTBEGIN
#define HUSH_ASSERT(condition, fmtFormat, ...)                                                                         \
	[[unlikely]]                                                                                                       \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		Hush::LogFormat(Hush::ELogLevel::Critical, "Assertion error at {} line {}! " fmtFormat, __FILE__, __LINE__,    \
						##__VA_ARGS__);                                                                                \
		HUSH_DEBUG_BREAK;                                                                                              \
	}
#else
// noop
#define HUSH_ASSERT(condition, fmtFormat, ...) static_cast<void>(condition) // To avoid unused variable warnings
#endif
#define HUSH_RESULT_ASSERT(result, message, ...)                                                                       \
	HUSH_ASSERT(result.has_value(), "{} error: {}", message, magic_enum::enum_name(result.error()))

#define HUSH_COND_FAIL_V(condition, retval)                                                                            \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		return retval;                                                                                                 \
	}

#define HUSH_COND_FAIL_MSG(condition, fmtFormat, ...)                                                                  \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		Hush::LogFormat(Hush::ELogLevel::Error, "Condition failed at {} line {}! " fmtFormat, __FILE__, __LINE__,      \
						##__VA_ARGS__);                                                                                \
		return;                                                                                                        \
	}

#define HUSH_COND_FAIL_MSG_V(condition, retval, fmtFormat, ...)                                                        \
	if (!(condition))                                                                                                  \
	{                                                                                                                  \
		Hush::LogFormat(Hush::ELogLevel::Error, "Condition failed at {} line {}! " fmtFormat, __FILE__, __LINE__,      \
						##__VA_ARGS__);                                                                                \
		return retval;                                                                                                 \
	}

#define HUSH_STATIC_ASSERT(condition, ...) static_assert(condition, #__VA_ARGS__)

// NOLINTEND
