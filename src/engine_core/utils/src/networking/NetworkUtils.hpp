/*! \file NetworkUtils.hpp
	\author Kyn21kx
	\date 2024-05-25
	\brief Provides simple utility functions for network/web related technologies
*/

#pragma once
#include "Platform.hpp"
#include <StringUtils.hpp>

namespace Hush::Networking
{

#if HUSH_PLATFORM_WIN
	template <uint32_t N>
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	constexpr auto SystemOpenURL(const char (&url)[N])
	{
		const char *cmd = StringUtils::CompileTimeConcat("start ", url).data();
		return system(cmd);
	}
#elif HUSH_PLATFORM_OSX
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	constexpr auto SystemOpenURL(const char (&url)[N])
	{
		const char *cmd = StringUtils::CompileTimeConcat("open ", url).data();
		return system(cmd);
	}
#elif HUSH_PLATFORM_LINUX
	// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
	constexpr auto SystemOpenURL(const char (&url)[N])
	{
		const char *cmd = StringUtils::CompileTimeConcat("xdg-open ", url).data();
		return system(cmd);
	}
#elif HUSH_PLATFORM_EMSCRIPTEN
#include <emscripten/emscripten.h>
	template <uint32_t N>
	constexpr auto SystemOpenURL(const char (&url)[N])
	{
		EM_ASM_(
			{
				const url = UTF8ToString($0);
				window.open(url, '_blank');
			},
			url);
		return 0; // No meaningful return value for Emscripten
	}
#else
#error "Unknown compiler"
#endif
} // namespace Hush::Networking
