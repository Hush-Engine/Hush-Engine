/*! \file ThreadPool.test.cpp
	\author Alan Ramirez
	\date 2025-01-04
	\brief ThreadPool tests
*/

#include "utils/ParallelUtils.hpp"
#include "executors/ThreadPool.hpp"
#include "async/WhenAll.hpp"

#include <Logger.hpp>
#include <catch2/catch_test_macros.hpp>
#include <fstream>
#include <random>
#include <set>

TEST_CASE("Threadpool creation", "[threadpool]")
{
	using namespace Hush::Threading;
	using namespace Hush::Threading::Executors;

	static constexpr uint32_t NUM_THREADS = 4;

	ThreadPoolOptions options;
	options.numThreads = NUM_THREADS; // Use 4 threads for testing
	auto threadPool = ThreadPool::Create(options);

	REQUIRE(threadPool.GetNumThreads() == NUM_THREADS);
}

TEST_CASE("Threadpool single task execution", "[threadpool]")
{
	using namespace Hush::Threading;
	using namespace Hush::Threading::Executors;

	static constexpr uint32_t NUM_THREADS = 4;
	ThreadPoolOptions options;
	options.numThreads = NUM_THREADS;
	auto threadPool = ThreadPool::Create(options);

	bool taskExecuted = false;

	auto taskFunc = [](bool &executed) -> Task<void> {
		executed = true;
		co_return;
	};

	Task<void> task = Hush::Threading::Executors::RunOn(&threadPool, taskFunc(taskExecuted));

	Hush::Threading::Wait(task);

	REQUIRE(taskExecuted);
}

TEST_CASE("Threadpool parallel task execution", "[threadpool]")
{
	using namespace Hush::Threading;
	using namespace Hush::Threading::Executors;

	static constexpr uint32_t NUM_THREADS = 4;
	static constexpr size_t NUM_TASKS = 5000;

	ThreadPoolOptions options;
	options.numThreads = NUM_THREADS;
	auto threadPool = ThreadPool::Create(options);

	std::set<size_t> threadIds;
	std::mutex mutex;
	std::vector<Task<void>> tasks;
	tasks.reserve(NUM_TASKS);

	auto taskFunc = [](std::set<uint64_t> &threadIds, std::mutex &mutex) -> Task<void> {
		const auto currentThreadId = std::hash<std::thread::id>()(std::this_thread::get_id());

		std::unique_lock<std::mutex> lock(mutex);
		threadIds.insert(currentThreadId);

		co_return;
	};

	for (size_t i = 0; i < NUM_TASKS; ++i)
	{
		tasks.push_back(Hush::Threading::Executors::RunOn(&threadPool, taskFunc(threadIds, mutex)));
	}

	for (auto &task : tasks)
	{
		Hush::Threading::Wait(task);
	}
	REQUIRE(threadIds.size() > 0);
	REQUIRE(threadIds.size() <= NUM_THREADS);
	for (const auto &threadId : threadIds)
	{
		Hush::LogFormat(Hush::ELogLevel::Info, "Thread ID: {}", threadId);
	}
}

TEST_CASE("Threadpool WhenAll", "[threadpool]")
{
	using namespace Hush::Threading;
	using namespace Hush::Threading::Executors;

	static constexpr uint32_t NUM_THREADS = 4;
	static constexpr size_t NUM_TASKS = 4;

	ThreadPoolOptions options;
	options.numThreads = NUM_THREADS;
	auto threadPool = ThreadPool::Create(options);

	std::vector<Task<void>> tasks;
	tasks.reserve(NUM_TASKS);

	auto taskFunc = []() -> Task<void> {
		std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Simulate work
		co_return;
	};

	for (size_t i = 0; i < NUM_TASKS; ++i)
	{
		tasks.push_back(Hush::Threading::Executors::RunOn(&threadPool, taskFunc()));
	}

	std::chrono::time_point<std::chrono::steady_clock> start = std::chrono::steady_clock::now();
	Wait(WhenAll(std::move(tasks)));
	std::chrono::time_point<std::chrono::steady_clock> end = std::chrono::steady_clock::now();

	// Since we have 4 threads, the total time should be around 10ms, but it may vary slightly due to scheduling.
	// So, let's check that the total time is less than 30ms to account for some overhead.
	REQUIRE(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() < 30);
}

TEST_CASE("Threadpool parallel for", "[threadpool]")
{
	using namespace Hush::Threading;
	using namespace Hush::Threading::Executors;

	static constexpr uint32_t NUM_THREADS = 4;
	static constexpr size_t NUM_ELEMENTS = 10000;

	ThreadPoolOptions options;
	options.numThreads = NUM_THREADS;
	ThreadPool threadPool = ThreadPool::Create(options);

	std::vector<int> results;
	results.reserve(NUM_ELEMENTS);
	for (int i = 1; i <= NUM_ELEMENTS; ++i)
	{
		results.push_back(i);
	}
	auto taskFunc = [](int &value) {
		value += 1;
	};

	Wait(ParallelFor(&threadPool, results.begin(), results.end(), taskFunc));

	for (int i = 1; i <= NUM_ELEMENTS; ++i)
	{
		REQUIRE(results[i - 1] == i + 1);
	}
}
