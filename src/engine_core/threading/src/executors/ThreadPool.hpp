/*! \file ThreadPool.hpp
	\author Alan Ramirez
	\date 2025-07-01
	\brief Threadpool implementation
*/

#pragma once

#include <thread>
#include <atomic>
#include <vector>
#include <async/Task.hpp>
#include <barrier>
#include <mutex>
#include "async/Executor.hpp"

namespace Hush::Threading
{
	class TaskOperation;
}
namespace Hush::Threading::Executors
{
	struct ThreadPoolOptions
	{
		uint32_t numThreads = std::thread::hardware_concurrency(); // Default to number of hardware threads
		bool pinToCore = false;									   // Whether to pin threads to cores (default: false)
	};

	class WorkerThread;

	class TaskOperation
	{
		friend class ThreadPool;
		friend class WorkerThread;

		explicit TaskOperation(ThreadPool *executor)
			: m_executor(executor)
		{
		}

	public:
		bool await_ready() const noexcept
		{
			return false;
		}

		void await_suspend(std::coroutine_handle<Task<void>::promise_type> awaitingCoroutine) noexcept;

		void await_resume() noexcept
		{
		}

	private:
		ThreadPool *m_executor;
		std::coroutine_handle<> m_awaitingCoroutine = nullptr;
	};

	class ThreadPool
	{

		ThreadPool(ThreadPoolOptions options = ThreadPoolOptions());

	public:
		~ThreadPool();

		TaskOperation Schedule();

		[[nodiscard]]
		uint32_t GetNumThreads() const noexcept
		{
			return static_cast<uint32_t>(m_threads.size());
		}

		static ThreadPool Create(ThreadPoolOptions options = ThreadPoolOptions());

	private:
		friend class WorkerThread;
		friend class TaskOperation;

		void PushWork(TaskOperation *task_operation);

		void NotifyWorkers();

		template <typename F>
		void LockAndExecuteTasksQueueOp(F &&stealFunc)
		{
			std::lock_guard lock(m_globalTasksMutex);
			stealFunc(m_globalTasks);
		}

		std::mutex m_globalTasksMutex; // Mutex to protect the global task queue
		std::vector<std::unique_ptr<WorkerThread>> m_threads;
		std::vector<TaskOperation *> m_globalTasks; // Global task queue for stealing
		std::barrier<> m_threadsBarrier;			// Used to synchronize thread start/stop
	};
} // namespace Hush::Threading::Executors