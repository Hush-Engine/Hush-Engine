/*! \file ThreadLocalMemoryResourcePool.test.cpp
	\author Alan Ramirez Herrera
	\date 2026-07-01
	\brief Tests for ThreadLocalMemoryResourcePool.
*/

#include "Hush/Memory/ThreadLocalMemoryResourcePool.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>

using Hush::Memory::ThreadLocalMemoryResourcePool;

namespace
{
	bool IsAligned(const void *ptr, std::size_t alignment) noexcept
	{
		return (reinterpret_cast<std::uintptr_t>(ptr) % alignment) == 0;
	}

	// Consumes an allocation whose pointer the test doesn't need (it only inspects stats),
	// silencing MSVC C4858 (discarding a raw-pointer allocation result).
	void Consume(void * /*ptr*/) noexcept
	{
	}
} // namespace

TEST_CASE("Frame pool basic allocation is usable and aligned", "[memory]")
{
	ThreadLocalMemoryResourcePool pool;

	void *a = pool.allocate(64, alignof(std::max_align_t));
	void *b = pool.allocate(128, alignof(std::max_align_t));

	REQUIRE(a != nullptr);
	REQUIRE(b != nullptr);
	REQUIRE(a != b);
	REQUIRE(IsAligned(a, alignof(std::max_align_t)));
	REQUIRE(IsAligned(b, alignof(std::max_align_t)));

	// The memory must be writable.
	std::memset(a, 0xAB, 64);
	std::memset(b, 0xCD, 128);
}

TEST_CASE("Frame pool honours over-alignment", "[memory]")
{
	ThreadLocalMemoryResourcePool pool;

	for (int i = 0; i < 8; ++i)
	{
		void *p = pool.allocate(1, 64);
		REQUIRE(p != nullptr);
		REQUIRE(IsAligned(p, 64));
	}
}

TEST_CASE("Reset rewinds the initial buffer for reuse", "[memory]")
{
	// Small initial buffer so we stay within it.
	ThreadLocalMemoryResourcePool pool(/*initialArenaSize=*/1024);

	void *first = pool.allocate(64, alignof(std::max_align_t));
	void *second = pool.allocate(64, alignof(std::max_align_t));
	REQUIRE(second != first);

	pool.Reset();

	// After a reset the arena starts over from the beginning of the same buffer, so the first
	// allocation of the same size/alignment lands at the same address.
	void *afterReset = pool.allocate(64, alignof(std::max_align_t));
	REQUIRE(afterReset == first);
}

TEST_CASE("Allocations larger than the initial buffer spill to upstream", "[memory]")
{
	ThreadLocalMemoryResourcePool pool(/*initialArenaSize=*/256);

	// Much larger than the initial buffer: must still succeed via the upstream resource.
	void *big = pool.allocate(4096, alignof(std::max_align_t));
	REQUIRE(big != nullptr);
	std::memset(big, 0x11, 4096);

	// Reset must reclaim the spillover without crashing, and keep working afterwards.
	pool.Reset();
	void *again = pool.allocate(4096, alignof(std::max_align_t));
	REQUIRE(again != nullptr);
}

TEST_CASE("Concurrent allocation from many threads then reset at a barrier", "[memory]")
{
	ThreadLocalMemoryResourcePool pool(/*initialArenaSize=*/4096);

	constexpr int NUM_THREADS = 8;
	constexpr int ALLOCS_PER_THREAD = 256;

	std::vector<std::thread> threads;
	threads.reserve(NUM_THREADS);
	for (int t = 0; t < NUM_THREADS; ++t)
	{
		threads.emplace_back([&pool] {
			for (int i = 0; i < ALLOCS_PER_THREAD; ++i)
			{
				void *p = pool.allocate(48, alignof(std::max_align_t));
				REQUIRE(p != nullptr);
				// Touch the memory to catch overlapping allocations under a sanitizer.
				std::memset(p, 0x7F, 48);
			}
		});
	}
	for (std::thread &thread : threads)
	{
		thread.join();
	}

	// All workers are joined (quiescent): resetting is safe here.
	pool.Reset();

	void *p = pool.allocate(48, alignof(std::max_align_t));
	REQUIRE(p != nullptr);
}

TEST_CASE("GetStats reports per-cycle usage without spilling within the buffer", "[memory]")
{
	constexpr std::size_t ARENA = 4096;
	ThreadLocalMemoryResourcePool pool(ARENA);

	// Three allocations well within the initial buffer.
	Consume(pool.allocate(64, alignof(std::max_align_t)));
	Consume(pool.allocate(128, alignof(std::max_align_t)));
	Consume(pool.allocate(32, alignof(std::max_align_t)));

	pool.Reset();

	const auto stats = pool.GetStats();
	REQUIRE(stats.bytesRequestedLastCycle == 64 + 128 + 32);
	REQUIRE(stats.allocationCountLastCycle == 3);
	REQUIRE(stats.spilledBytesLastCycle == 0);
	REQUIRE(stats.spilledAllocationsLastCycle == 0);
	REQUIRE(stats.highWaterBytes == 64 + 128 + 32);
	REQUIRE(stats.arenaCount == 1);
	REQUIRE(stats.initialArenaSize == ARENA);
}

TEST_CASE("GetStats reports spillover when allocations exceed the buffer", "[memory]")
{
	constexpr std::size_t ARENA = 256;
	ThreadLocalMemoryResourcePool pool(ARENA);

	// Comfortably larger than the initial buffer: the arena must pull from upstream.
	Consume(pool.allocate(4096, alignof(std::max_align_t)));
	pool.Reset();

	const auto stats = pool.GetStats();
	REQUIRE(stats.bytesRequestedLastCycle == 4096);
	REQUIRE(stats.allocationCountLastCycle == 1);
	REQUIRE(stats.spilledBytesLastCycle > 0);
	REQUIRE(stats.spilledAllocationsLastCycle > 0);
}

TEST_CASE("GetStats high-water mark is the running maximum across cycles", "[memory]")
{
	ThreadLocalMemoryResourcePool pool(4096);

	Consume(pool.allocate(512, alignof(std::max_align_t)));
	pool.Reset();
	REQUIRE(pool.GetStats().highWaterBytes == 512);

	// A bigger cycle raises the high-water mark.
	Consume(pool.allocate(1024, alignof(std::max_align_t)));
	pool.Reset();
	REQUIRE(pool.GetStats().highWaterBytes == 1024);

	// A smaller cycle does not lower it, and the per-cycle figure reflects only this cycle.
	Consume(pool.allocate(128, alignof(std::max_align_t)));
	pool.Reset();
	const auto stats = pool.GetStats();
	REQUIRE(stats.bytesRequestedLastCycle == 128);
	REQUIRE(stats.highWaterBytes == 1024);
}
