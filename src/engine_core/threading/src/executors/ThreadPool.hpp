/*! \file ThreadPool.hpp
	\author Alan Ramirez
	\date 2025-07-01
	\brief Threadpool implementation
*/

#pragma once

#include <thread>
#include <vector>
#include <async/Task.hpp>
#include <barrier>
#include <mutex>

// NOLINTBEGIN(readability-identifier-naming,modernize-use-nodiscard)

namespace Hush::Threading::Executors
{
	struct ThreadPoolOptions
	{
		uint32_t numThreads = std::thread::hardware_concurrency(); // Default to number of hardware threads
		bool pinToCore = false;									   // Whether to pin threads to cores (default: false)
	};

	class WorkerThread;
	class ThreadPool;

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
		ThreadPool(const ThreadPool &other) = delete;
		ThreadPool &operator=(const ThreadPool &other) = delete;
		ThreadPool(ThreadPool &&other) noexcept = delete;
		ThreadPool &operator=(ThreadPool &&other) noexcept = delete;
		ThreadPool() = delete;

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

		void PushWork(TaskOperation *taskOperation);

		void NotifyWorkers();

		template <typename F>
		void LockAndExecuteTasksQueueOp(F stealFunc)
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

// NOLINTEND(readability-identifier-naming,modernize-use-nodiscard)
