/*! \file StringAllocation.hpp
	\author Alan Ramirez Herrera
	\date 2026-07-01
	\brief Helpers for materializing null-terminated strings from arbitrary string views,
		   backed by a std::pmr memory resource (typically a frame or scene arena).
*/

#pragma once

#include "Assertions.hpp"
#include "NullTerminatedStringView.hpp"

#include <cstring>
#include <memory_resource>
#include <string_view>

namespace Hush
{
	/// Allocates a null-terminated copy of `sv` from `mr` and returns a view over it.
	///
	/// This is the bridge for the common case where an arbitrary — possibly non-null-terminated
	/// — `std::string_view` must be handed to a C API (Flecs, Win32, ...) that requires a
	/// `const char*`. Backing it with a frame or scene arena makes the copy a pointer bump that
	/// is reclaimed in bulk, instead of a per-call `std::string` heap allocation.
	///
	/// The returned view is valid for as long as `mr` keeps the allocation alive — for a
	/// monotonic frame/scene resource, until its next `Reset()`. The bytes are never freed
	/// individually.
	///
	/// @param sv The (possibly non-null-terminated) source view to copy.
	/// @param mr The memory resource to allocate from. Must not be null.
	[[nodiscard]]
	inline NullTerminatedStringView MakeNullTerminated(std::string_view sv, std::pmr::memory_resource *mr)
	{
		HUSH_ASSERT(mr != nullptr, "MakeNullTerminated requires a non-null memory resource");

		// +1 for the null terminator.
		char *buffer = static_cast<char *>(mr->allocate(sv.size() + 1, alignof(char)));
		if (!sv.empty())
		{
			std::memcpy(buffer, sv.data(), sv.size());
		}
		buffer[sv.size()] = '\0';

		return NullTerminatedStringView::promise_null_terminated(std::string_view{buffer, sv.size()});
	}
} // namespace Hush
