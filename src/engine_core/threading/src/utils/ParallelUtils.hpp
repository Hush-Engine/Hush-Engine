/*! \file ParallelUtils.hpp
	\author Alan Ramirez
	\date 2025-06-28
	\brief Parallel utils (such as for-loops)
*/

#pragma once

#include "async/SyncWait.hpp"
#include "async/Task.hpp"
#include "async/WhenAll.hpp"
#include "executors/ThreadPool.hpp"

namespace Hush::Threading
{
	static constexpr size_t MAX_PARALLEL_TASKS = 1024;
	static constexpr size_t MIN_CHUNK_SIZE = 1000;

	template <typename It, typename Fn>
		requires(std::is_invocable_v<Fn, std::add_lvalue_reference_t<typename std::iterator_traits<It>::value_type>>)
	Task<void> ParallelFor(Hush::Threading::Concepts::Executor auto *executor, It begin, It end, Fn &&function)
	{
		// We need to calculate how many tasks we are going to create.
		// For this, we need to split the range into chunks.
		// For this, I would like to use a simple heuristic:
		// - At most, we will create 1024 tasks to avoid overwhelming the thread pool. This is a hard limit.
		// - Each task will process at minimum, 1000 elements. This is a soft limit and can increase if the next
		// chunk is smaller than 1000 elements.

		std::size_t size = std::distance(begin, end);
		if (size == 0)
		{
			co_return;
		}

		const std::size_t numTasks = std::min<size_t>(MAX_PARALLEL_TASKS, std::max<size_t>(1, size / MIN_CHUNK_SIZE));
		const std::size_t chunkSize = size / numTasks;

		std::vector<Task<void>> tasks;
		It current = begin;

		while (current < end)
		{
			// First, we need to calculate the next iterator position, which is just the current iterator plus the
			// chunk size.
			It next = std::next(current, chunkSize);
			if (next >= end)
			{
				next = end;
			}
			else
			{
				// Now, we need to calculate the next-next, if that is greater than the end, our current next will
				// be the end.
				It nextNext = std::next(next, chunkSize);
				if (nextNext > end)
				{
					next = end;
				}
			}
			if (next == current)
			{
				break;
			}

			// Now we can create a task that will process the current chunk.
			auto forTask = [](auto function, It begin, It end) -> Task<void> {
				It current = begin;
				while (current != end)
				{
					function(*current);
					++current;
				}
				co_return;
			};

			Task<void> task = RunOn(executor, forTask(std::forward<Fn>(function), current, next));
			tasks.push_back(std::move(task));

			current = next;
		}
		if (tasks.empty())
		{
			co_return; // No tasks to run
		}
		// Now we can wait for all tasks to finish.
		co_await Hush::Threading::WhenAll(std::move(tasks));
	}
} // namespace Hush::Threading