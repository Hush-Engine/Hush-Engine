/*! \file CoroutineFrameResource.test.cpp
	\author Alan Ramirez Herrera
	\date 2026-07-01
	\brief Tests for the coroutine frame memory resource.
*/

#include "Hush/Memory/CoroutineFrameResource.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <new>
#include <thread>

TEST_CASE("Coroutine frame resource is a stable singleton", "[memory]")
{
	std::pmr::memory_resource *a = Hush::Memory::CoroutineFrameResource();
	std::pmr::memory_resource *b = Hush::Memory::CoroutineFrameResource();
	REQUIRE(a != nullptr);
	REQUIRE(a == b);
}

TEST_CASE("Coroutine frame resource allocate/deallocate on the same thread", "[memory]")
{
	std::pmr::memory_resource *mr = Hush::Memory::CoroutineFrameResource();

	void *p = mr->allocate(96, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	REQUIRE(p != nullptr);
	std::memset(p, 0x5A, 96);
	mr->deallocate(p, 96, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
}

TEST_CASE("Coroutine frame resource allows cross-thread deallocation", "[memory]")
{
	// Work-stealing means a coroutine frame allocated on one thread may be freed on another.
	// The resource must tolerate that; allocate on this thread, free on a worker thread.
	std::pmr::memory_resource *mr = Hush::Memory::CoroutineFrameResource();

	constexpr std::size_t SIZE = 128;
	void *p = mr->allocate(SIZE, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
	REQUIRE(p != nullptr);
	std::memset(p, 0x33, SIZE);

	std::thread worker([mr, p] { mr->deallocate(p, SIZE, __STDCPP_DEFAULT_NEW_ALIGNMENT__); });
	worker.join();

	SUCCEED("cross-thread deallocation did not corrupt the resource");
}

TEST_CASE("Coroutine frame resource under concurrent churn", "[memory]")
{
	std::pmr::memory_resource *mr = Hush::Memory::CoroutineFrameResource();

	constexpr int NUM_THREADS = 8;
	constexpr int ITERATIONS = 512;

	std::vector<std::thread> threads;
	threads.reserve(NUM_THREADS);
	for (int t = 0; t < NUM_THREADS; ++t)
	{
		threads.emplace_back([mr] {
			for (int i = 0; i < ITERATIONS; ++i)
			{
				const std::size_t size = 16 + static_cast<std::size_t>(i % 240);
				void *p = mr->allocate(size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
				REQUIRE(p != nullptr);
				std::memset(p, 0x1, size);
				mr->deallocate(p, size, __STDCPP_DEFAULT_NEW_ALIGNMENT__);
			}
		});
	}
	for (std::thread &thread : threads)
	{
		thread.join();
	}

	SUCCEED("concurrent allocate/deallocate churn completed");
}
