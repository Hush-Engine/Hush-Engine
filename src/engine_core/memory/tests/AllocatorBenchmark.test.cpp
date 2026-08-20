/*! \file AllocatorBenchmark.test.cpp
	\author Alan Ramirez Herrera
	\date 2026-07-01
	\brief Microbenchmarks for the memory allocators. Hidden by default ([.]); run with:
		   HushMemoryTest "[benchmark]"

	The coroutine-pool numbers answer the open question in CoroutineFrameResource.hpp: is the
	pooled resource actually faster than plain operator new (which mimalloc already backs)?
*/

#include "Hush/Memory/CoroutineFrameResource.hpp"
#include "Hush/Memory/ThreadLocalMemoryResourcePool.hpp"

#include <catch2/benchmark/catch_benchmark.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <new>
#include <vector>

#if defined(HUSH_USE_MIMALLOC)
#include <mimalloc.h>
#endif

using Hush::Memory::CoroutineFrameResource;
using Hush::Memory::ThreadLocalMemoryResourcePool;

TEST_CASE("Benchmark: coroutine frame alloc+free vs operator new", "[.][benchmark]")
{
	constexpr std::size_t ALIGN = __STDCPP_DEFAULT_NEW_ALIGNMENT__;
	std::pmr::memory_resource *pool = CoroutineFrameResource();

	// 96 B and 160 B bracket the measured hot coroutine frame sizes in an optimized build
	// (payloads 96/128/160 are >99.9% of frames). Debug frames are larger (~192-272 B), but the
	// pool only runs in optimized/Emscripten builds, so these are the representative sizes.
	BENCHMARK("coroutine pool: alloc+free 96B")
	{
		void *p = pool->allocate(96, ALIGN);
		pool->deallocate(p, 96, ALIGN);
		return p;
	};

	BENCHMARK("operator new: alloc+free 96B")
	{
		void *p = ::operator new(96);
		::operator delete(p, 96);
		return p;
	};

	BENCHMARK("coroutine pool: alloc+free 160B")
	{
		void *p = pool->allocate(160, ALIGN);
		pool->deallocate(p, 160, ALIGN);
		return p;
	};

	BENCHMARK("operator new: alloc+free 160B")
	{
		void *p = ::operator new(160);
		::operator delete(p, 160);
		return p;
	};

#if defined(HUSH_USE_MIMALLOC)
	// The real baseline: un-pooled coroutine frames hit mimalloc via the global operator new
	// override. (::operator new above is CRT malloc here — the test exe is not minject'd.)
	BENCHMARK("mimalloc: alloc+free 96B")
	{
		void *p = mi_malloc(96);
		mi_free(p);
		return p;
	};

	BENCHMARK("mimalloc: alloc+free 160B")
	{
		void *p = mi_malloc(160);
		mi_free(p);
		return p;
	};
#endif
}

TEST_CASE("Benchmark: frame arena batch-bump vs operator new", "[.][benchmark]")
{
	constexpr std::size_t ALIGN = alignof(std::max_align_t);
	constexpr int BATCH = 256;

	// Constructed once (owns a 1 MB buffer); each iteration bumps BATCH times then rewinds.
	ThreadLocalMemoryResourcePool pool(std::size_t{1} * 1024 * 1024);

	BENCHMARK("frame arena: 256x64B bump + reset")
	{
		void *last = nullptr;
		for (int i = 0; i < BATCH; ++i)
		{
			last = pool.allocate(64, ALIGN);
		}
		pool.Reset();
		return last;
	};

	BENCHMARK("operator new: 256x64B new + delete")
	{
		std::vector<void *> ptrs;
		ptrs.reserve(BATCH);
		for (int i = 0; i < BATCH; ++i)
		{
			ptrs.push_back(::operator new(64));
		}
		void *last = ptrs.empty() ? nullptr : ptrs.back();
		for (void *p : ptrs)
		{
			::operator delete(p, 64);
		}
		return last;
	};
}
