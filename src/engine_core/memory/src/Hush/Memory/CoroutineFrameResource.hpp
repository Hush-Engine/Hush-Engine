/*! \file CoroutineFrameResource.hpp
	\author Alan Ramirez Herrera
	\date 2026-07-01
	\brief Process-wide, cross-thread-safe memory resource for coroutine frame allocations,
		   plus a helper macro to route a promise type's operator new/delete through it.
*/

#pragma once

#include <cstddef>
#include <memory_resource>
#include <new>

namespace Hush::Memory
{
	/// Returns the process-wide memory resource used for coroutine frame allocations.
	///
	/// Coroutine frames are allocated and freed *individually*, and — because the engine's
	/// thread pool is work-stealing — a frame allocated on one thread may be destroyed on a
	/// different thread (e.g. a detached `SelfDeleteTask` that self-destroys in
	/// `final_suspend`). The returned resource is therefore safe for concurrent, cross-thread
	/// allocate/deallocate.
	///
	/// ## Implementation (thread-sharded free-list — the mimalloc pattern)
	/// Each thread gets its own shard of size-classed free-lists. Same-thread `allocate`/
	/// `deallocate` hit those free-lists with **no atomics** (the hot path for the synchronous
	/// `ParallelFor` fan-out, where a frame is allocated and freed on the same worker). A frame
	/// freed by a *different* thread than allocated it is pushed onto the owning shard's atomic
	/// MPSC "foreign free" list; the owner reclaims that list in a single atomic swap the next
	/// time it needs a block of that size class. Shards and their slabs are intentionally leaked
	/// (process lifetime), so a frame freed after its allocating thread exits is still valid.
	///
	/// @note Optimized for the coroutine-frame use case: alignment up to `max_align_t`. Requests
	///       whose payload exceeds the largest size class fall back to the upstream resource
	///       (`new_delete` -> mimalloc). Over-aligned requests are not expected from coroutine
	///       frames and are asserted against in debug.
	[[nodiscard]]
	std::pmr::memory_resource *CoroutineFrameResource() noexcept;
} // namespace Hush::Memory

/// Injects a coroutine-frame `operator new`/`operator delete` pair into a promise type,
/// routing frame allocations through `Hush::Memory::CoroutineFrameResource()`.
///
/// The C++ coroutine machinery calls the *sized* `operator delete` with the same size it
/// passed to `operator new`, so the pointer/size/alignment triple always round-trips
/// correctly through the pmr resource. Place this inside the body of every promise type
/// whose frames should be pooled.
///
/// **Conditional:** when `HUSH_USE_MIMALLOC` is defined, mimalloc already backs the global
/// `operator new` and — measured — is faster than this pool at real coroutine frame sizes, so
/// the macro expands to nothing and frames use global `operator new` (= mimalloc) directly. The
/// pool is used only where mimalloc is absent (e.g. Emscripten), where it beats the default
/// allocator. See AllocatorBenchmark for the numbers.
#if defined(HUSH_USE_MIMALLOC)
#define HUSH_COROUTINE_FRAME_ALLOCATOR()
#else
#define HUSH_COROUTINE_FRAME_ALLOCATOR()                                                                               \
	static void *operator new(std::size_t hushFrameSize)                                                               \
	{                                                                                                                  \
		return ::Hush::Memory::CoroutineFrameResource()->allocate(hushFrameSize, __STDCPP_DEFAULT_NEW_ALIGNMENT__);    \
	}                                                                                                                  \
	static void operator delete(void *hushFramePtr, std::size_t hushFrameSize) noexcept                                \
	{                                                                                                                  \
		::Hush::Memory::CoroutineFrameResource()->deallocate(hushFramePtr, hushFrameSize,                              \
															 __STDCPP_DEFAULT_NEW_ALIGNMENT__);                        \
	}
#endif
