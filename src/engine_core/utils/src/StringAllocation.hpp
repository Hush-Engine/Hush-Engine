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
	/// Use this function when you need to pass a string view to an API that requires a null-terminated string, 
	/// and you want to avoid expensive allocations through the global heap.
	/// The returned view is valid until the memory resource is reset or destroyed.
	/// 
	/// The most common memory resource for this is the frame-scoped allocator, and for strings that are meant to be used for
	/// short-lived operations.
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
