/*! \file ThreadLocalMemoryResourcePool.hpp
	\author Alan Ramirez Herrera
	\date 2026-07-01
	\brief A thread-safe, resettable bump allocator (std::pmr::memory_resource) for
		   frame- and scene-scoped temporary allocations.
*/

#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <memory_resource>
#include <mutex>
#include <optional>
#include <vector>

namespace Hush::Memory
{
	/// A thread-safe, resettable bump allocator built on top of `std::pmr`.
	///
	/// Every thread that allocates from this resource is handed its own
	/// `std::pmr::monotonic_buffer_resource`, backed by an owned initial buffer. Allocation
	/// is a pointer bump (there is no per-allocation deallocation); `Reset()` rewinds every
	/// thread's arena at once, reusing the initial buffers so no memory is returned to the
	/// upstream resource on the common path.
	///
	/// It is intended to back the engine's frame- and scene-scoped memory resources
	/// (see `HushEngine::GetFrameScopeMemoryResource` / `GetSceneScopeAllocator`).
	///
	/// ## Threading contract
	/// - `do_allocate` / `do_deallocate` are safe to call concurrently from many threads:
	///   each call only touches the calling thread's own arena. A short mutex is taken only
	///   the first time a given thread allocates from a given pool (to register its arena).
	/// - `Reset()` must ONLY be called at a barrier where no other thread is allocating from
	///   this resource (e.g. the end-of-frame synchronization point, or scene teardown). It
	///   walks and rewinds every thread's arena, which would race with a concurrent
	///   allocation.
	///
	/// ## Lifetime
	/// Instances are expected to outlive every thread that allocates from them (they are
	/// members of the engine and live for the whole engine lifetime). The per-thread arena
	/// lookup caches a raw pointer keyed by the pool instance; destroying a pool while a
	/// thread still holds a cached entry for it (and then reusing the same address for a new
	/// pool) would be undefined. This does not happen for the engine-owned pools.
	class ThreadLocalMemoryResourcePool final : public std::pmr::memory_resource
	{
	public:
		/// Default size, in bytes, of each thread's initial (reused) buffer.
		static constexpr std::size_t DEFAULT_ARENA_SIZE = std::size_t{256} * 1024;

		/// A snapshot of the pool's usage, produced at each `Reset()`. All figures are for the
		/// cycle that just ended (a frame for the frame pool, a scene's lifetime for the scene
		/// pool), except `highWaterBytes`, which is the running maximum across all cycles.
		///
		/// `bytesRequestedLastCycle` counts the sizes passed to `allocate` (excludes alignment
		/// padding). `spilledBytesLastCycle` counts what the arenas actually pulled from the
		/// upstream resource when their initial buffers were exhausted — i.e. `spilledBytes > 0`
		/// means the arena size is too small for that cycle's peak.
		struct Stats
		{
			std::size_t bytesRequestedLastCycle;
			std::size_t allocationCountLastCycle;
			std::size_t spilledBytesLastCycle;
			std::size_t spilledAllocationsLastCycle;
			std::size_t highWaterBytes;
			std::size_t arenaCount;
			std::size_t initialArenaSize;
		};

		/// @param initialArenaSize Size of each thread's reusable initial buffer, in bytes.
		/// @param upstream Resource used for spillover past the initial buffer. Defaults to
		///        `new_delete_resource`, which routes through mimalloc via the global
		///        `operator new` overrides.
		explicit ThreadLocalMemoryResourcePool(
			std::size_t initialArenaSize = DEFAULT_ARENA_SIZE,
			std::pmr::memory_resource *upstream = std::pmr::new_delete_resource()) noexcept;

		ThreadLocalMemoryResourcePool(const ThreadLocalMemoryResourcePool &) = delete;
		ThreadLocalMemoryResourcePool &operator=(const ThreadLocalMemoryResourcePool &) = delete;
		ThreadLocalMemoryResourcePool(ThreadLocalMemoryResourcePool &&) = delete;
		ThreadLocalMemoryResourcePool &operator=(ThreadLocalMemoryResourcePool &&) = delete;

		~ThreadLocalMemoryResourcePool() override;

		/// Rewinds every thread's arena, reusing the initial buffers. Spillover blocks are
		/// released back to the upstream resource.
		///
		/// @warning See the threading contract in the class documentation: this must only be
		///          called at a barrier where no thread is allocating from this pool.
		void Reset();

		/// Returns the usage snapshot taken at the last `Reset()`. Intended to be read on the
		/// same thread that drives `Reset()` (e.g. the main loop) — it reads plain members that
		/// are written only inside `Reset()` at the barrier.
		[[nodiscard]]
		Stats GetStats() const noexcept;

	protected:
		void *do_allocate(std::size_t bytes, std::size_t alignment) override;
		void do_deallocate(void *ptr, std::size_t bytes, std::size_t alignment) noexcept override;
		[[nodiscard]]
		bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override;

	private:
		/// A `memory_resource` that forwards to a wrapped upstream while counting what passes
		/// through it. Every per-thread arena uses this as its monotonic resource's upstream, so
		/// it counts exactly the memory pulled from upstream when an initial buffer is exhausted
		/// (spillover). The atomics are touched only on that cold path.
		struct CountingUpstream final : std::pmr::memory_resource
		{
			explicit CountingUpstream(std::pmr::memory_resource *wrapped) noexcept
				: m_wrapped(wrapped)
			{
			}

			std::atomic<std::size_t> spilledBytes{0};
			std::atomic<std::size_t> spilledAllocations{0};

		protected:
			void *do_allocate(std::size_t bytes, std::size_t alignment) override
			{
				spilledBytes.fetch_add(bytes, std::memory_order_relaxed);
				spilledAllocations.fetch_add(1, std::memory_order_relaxed);
				return m_wrapped->allocate(bytes, alignment);
			}

			void do_deallocate(void *ptr, std::size_t bytes, std::size_t alignment) noexcept override
			{
				// Frees of spillover chunks (on Rewind/teardown) are not subtracted: the spill
				// counters measure per-cycle upstream traffic and are zeroed in Reset().
				m_wrapped->deallocate(ptr, bytes, alignment);
			}

			[[nodiscard]]
			bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override
			{
				return this == &other;
			}

		private:
			std::pmr::memory_resource *m_wrapped;
		};

		/// A single thread's arena: an owned buffer plus the monotonic resource over it.
		/// Stored behind a `unique_ptr` so it never moves after construction — the monotonic
		/// resource holds a pointer into `buffer`.
		struct Arena
		{
			Arena(std::size_t bufferSize, std::pmr::memory_resource *upstreamResource)
				: buffer(std::make_unique<std::byte[]>(bufferSize)),
				  size(bufferSize),
				  upstream(upstreamResource),
				  resource(std::in_place, buffer.get(), bufferSize, upstreamResource)
			{
			}

			/// Rewinds this arena for reuse. We destroy and reconstruct the monotonic resource
			/// over the same buffer rather than calling release(): that guarantees the bump
			/// pointer resets to the start of the initial buffer (release() is not portably
			/// guaranteed to rewind an externally-provided buffer). The destructor frees any
			/// upstream spillover blocks; the owned `buffer` persists across rewinds.
			void Rewind()
			{
				resource.emplace(buffer.get(), size, upstream);
			}

			std::unique_ptr<std::byte[]> buffer;
			std::size_t size;
			std::pmr::memory_resource *upstream;
			std::optional<std::pmr::monotonic_buffer_resource> resource;

			// Usage since the last Reset(). Written only by the owning thread in do_allocate;
			// read+zeroed by Reset() at the barrier. Plain (no atomics) — single writer.
			std::size_t bytesRequested = 0;
			std::size_t allocationCount = 0;
		};

		/// Returns (creating + registering on first use) the calling thread's arena.
		Arena &GetLocalArena();

		// Unique per-instance id used to key the thread-local arena cache/map. Using a
		// never-reused id (rather than `this`) avoids a false cache hit when a destroyed pool's
		// address is reused by a new pool (e.g. stack-allocated pools in tests).
		std::uint64_t m_generation;

		std::size_t m_initialArenaSize;
		std::pmr::memory_resource *m_upstream;
		// Wraps m_upstream to count spillover; every arena uses &m_countingUpstream as upstream.
		// Declared after m_upstream so it is constructed from an initialized pointer.
		CountingUpstream m_countingUpstream;

		// Owns every per-thread arena. Guarded by m_mutex for registration/reset/teardown.
		std::mutex m_mutex;
		std::vector<std::unique_ptr<Arena>> m_arenas;

		// Usage snapshot, written only inside Reset() (main thread, at the barrier).
		std::size_t m_lastBytesRequested = 0;
		std::size_t m_lastAllocationCount = 0;
		std::size_t m_lastSpilledBytes = 0;
		std::size_t m_lastSpilledAllocations = 0;
		std::size_t m_highWaterBytes = 0;
		std::size_t m_arenaCount = 0;
	};
} // namespace Hush::Memory
