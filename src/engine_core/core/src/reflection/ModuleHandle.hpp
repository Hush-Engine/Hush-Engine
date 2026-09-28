/*! \file ModuleHandle.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Module handle used to track which module owns a reflected type
*/

#pragma once

#include <cstdint>

namespace Hush
{
	/// Handle that identifies a loaded module. The engine itself is also
	/// treated as a module so built-in types have an owner too.
	using ModuleHandle = std::uint64_t;

	/// Handle of the engine module. Types registered without an explicit
	/// module belong to the engine and are never unloaded.
	inline constexpr ModuleHandle ENGINE_MODULE_HANDLE = 0;

	/// Invalid module handle, used as a "no module" value.
	inline constexpr ModuleHandle INVALID_MODULE_HANDLE = ~static_cast<ModuleHandle>(0);
} // namespace Hush
