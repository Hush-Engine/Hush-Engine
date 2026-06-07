/*! \file ThreadPool.cpp
	\author Alan Ramirez
	\date 2025-07-01
	\brief Threadpool implementation
*/

#include "ThreadPool.hpp"
#include "threadpool/StealingQueue.hpp"
#include <Platform.hpp>
#include <Profiling.hpp>
#include <random>
#include <semaphore>

#if HUSH_PLATFORM_WIN
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

static void SetCurrentThreadAffinity(std::uint32_t affinity)
{
#if HUSH_PLATFORM_WIN
	const DWORD_PTR mask = 1ULL << static_cast<std::uint64_t>(affinity);
	SetThreadAffinityMask(GetCurrentThread(), mask);
#endif
}

static void SetCurrentThreadName(const char *name)
{
#if HUSH_PLATFORM_WIN
	// Windows 10 1607 and later support setting thread names via SetThreadDescription
	HRESULT hr = SetThreadDescription(GetCurrentThread(), std::wstring(name, name + strlen(name)).c_str());
	if (FAILED(hr))
	{
		Hush::LogFormat(Hush::ELogLevel::Warn, "Failed to set thread name: {}", name);
	}
#endif
}

namespace Hush::Threading::Executors
{
	static constexpr size_t WORKER_QUEUE_SIZE = 256;		  // Size of the worker queue
	static constexpr size_t GLOBAL_QUEUE_STEALING_COUNT = 64; // Number of tasks to steal from the global queue

	// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
	thread_local WorkerThread *G_CURRENT_WORKER_THREAD = nullptr;

	class WorkerThread
	{
	public:
		WorkerThread(ThreadPool &threadpool, std::barrier<> &startBarrier, int32_t threadAffinity)
			: m_threadPool(threadpool)
		{
			// Build thread/fiber names before launching so they're available via the 'this' pointer.
			// Stored as members because Tracy requires name strings to remain valid for the
			// lifetime of the program.
			if (threadAffinity >= 0)
			{
				m_threadName = "WorkerThread-P" + std::to_string(threadAffinity);
			}

			// Create the worker thread
			m_thread = std::jthread([this, &startBarrier, threadAffinity](std::stop_token stopToken) {
				if (m_threadName.empty())
				{
					const auto threadId = std::hash<std::thread::id>{}(std::this_thread::get_id());
					m_threadName = "WorkerThread-" + std::to_string(threadId);
				}
				Hush::LogFormat(ELogLevel::Debug, "Worker thread started: {}", m_threadName);
				G_CURRENT_WORKER_THREAD = this;
				if (threadAffinity >= 0)
				{
					SetCurrentThreadAffinity(threadAffinity);
				}
				SetCurrentThreadName(m_threadName.c_str());
				tracy::SetThreadName(m_threadName.c_str());

				// Wait until the thread is started
				startBarrier.arrive_and_wait();

				WorkerFunction(stopToken);
			});
		}

		void SetStealers(std::vector<Stealer<TaskOperation *, WORKER_QUEUE_SIZE>> workers) noexcept
		{
			m_stealers = std::move(workers);

			// Randomly select a worker to start stealing from
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<std::uint32_t> dist(0, static_cast<std::uint32_t>(m_stealers.size() - 1));
			m_previousStealIndex = dist(gen);
		}

		void Wake() noexcept
		{
			// Notify the worker thread that there is work to do
			m_wakeUpSemaphore.release();
		}

		[[nodiscard]]
		Stealer<TaskOperation *, WORKER_QUEUE_SIZE> MakeStealer() noexcept
		{
			return m_workerQueue.MakeStealer();
		}

		void Stop() noexcept
		{
			// Request the thread to stop
			m_thread.request_stop();
		}

		void Join() noexcept
		{
			// Wait for the thread to finish
			if (m_thread.joinable())
			{
				m_thread.join();
			}
		}

	private:
		void WorkerFunction(std::stop_token stopToken)
		{
			static constexpr uint32_t busyLoopIterations = 1000;
			static constexpr size_t stealingCount = 16; // We will try to steal this many tasks at once

			while (!stopToken.stop_requested())
			{
				// First, we need to check if we can deque a task from our worker queue
				if (std::optional<TaskOperation *> taskResult = m_workerQueue.Pop(); taskResult.has_value())
				{
					TaskOperation *task = *taskResult;
					if (task != nullptr)
					{
						task->m_awaitingCoroutine.resume();
					}
					continue;
				}
				// If we reach here, it means we didn't find a task in our worker queue, so we need to steal from other
				// threads
				for (uint32_t i = 0; i < busyLoopIterations; ++i)
				{
					if (!m_stealers.empty())
					{
						// Try to steal from the next worker in the list
						auto &stealer = m_stealers[m_previousStealIndex];

						Result<std::tuple<TaskOperation *, size_t>, EStealError> result =
							stealer.StealAndPop(m_workerQueue, [](size_t count) {
								(void)count; // Unused parameter, but we need to keep the signature
								return stealingCount;
							});

						if (result.has_error())
						{
							// If we failed to steal, we can just continue to the next worker
							m_previousStealIndex = (m_previousStealIndex + 1) % m_stealers.size();
							continue;
						}

						// If we are here, it means we successfully stole some tasks and popped one
						auto [task, count] = result.value();
						if (task != nullptr)
						{
							task->m_awaitingCoroutine.resume();
						}

						// Move to the next worker in the list
						m_previousStealIndex = (m_previousStealIndex + 1) % m_stealers.size();
					}
				}

				// We have one last chance, we need to steal from the global queue
				std::array<TaskOperation *, GLOBAL_QUEUE_STEALING_COUNT> stealers;
				uint32_t stealCount = 0;

				m_threadPool.LockAndExecuteTasksQueueOp([&](std::vector<TaskOperation *> &globalTasks) {
					if (!globalTasks.empty())
					{
						while (stealCount < GLOBAL_QUEUE_STEALING_COUNT && !globalTasks.empty())
						{
							// Pop tasks from the global queue
							stealers[stealCount++] = globalTasks.back();
							globalTasks.pop_back();
						}
					}
				});

				if (stealCount > 0)
				{
					// Yey, we have some tasks, so we need to push them to our worker queue
					for (uint32_t i = 0; i < stealCount; ++i)
					{
						TaskOperation *task = stealers[i];
						if (task != nullptr)
						{
							m_workerQueue.Push(std::move(task));
						}
					}
				}
				else
				{
					// Unfortunately, we didn't find any tasks to execute, so we need to sleep until
					// someone wakes us up 😴😴😴
					m_wakeUpSemaphore.acquire();
				}
			}
		}

	private:
		friend class TaskOperation;

		std::vector<Stealer<TaskOperation *, WORKER_QUEUE_SIZE>> m_stealers;
		std::jthread m_thread;
		std::string m_threadName;
		Worker<TaskOperation *, WORKER_QUEUE_SIZE> m_workerQueue;
		ThreadPool &m_threadPool;
		std::uint32_t m_previousStealIndex{};
		std::binary_semaphore m_wakeUpSemaphore{0}; // Semaphore to wake up the thread when there is work to do
	};

	void TaskOperation::await_suspend(std::coroutine_handle<Task<void>::promise_type> awaitingCoroutine) noexcept
	{
		m_awaitingCoroutine = awaitingCoroutine;

		if (G_CURRENT_WORKER_THREAD != nullptr && G_CURRENT_WORKER_THREAD->m_workerQueue.Push(this))
		{
			// We are already in a worker thread and successfully pushed the task to the worker queue, so we
			// just finished.
			m_executor->NotifyWorkers();
			return;
		}

		// We are not in a worker thread or failed to push the task to the worker queue, so we need to push it to the
		// global queue.
		m_executor->PushWork(this);
	}

} // namespace Hush::Threading::Executors

Hush::Threading::Executors::ThreadPool::ThreadPool(ThreadPoolOptions options)
	: m_threadsBarrier(options.numThreads + 1) // +1 for the main thread to wait on
{
	// Get the number of cores. TODO: Use this to pin threads to cores if options.pinToCore is true.

	// Create worker threads
	for (uint32_t i = 0; i < options.numThreads; i++)
	{
		int32_t threadAffinity = options.pinToCore ? static_cast<int32_t>(i) : -1;
		m_threads.emplace_back(std::make_unique<WorkerThread>(*this, m_threadsBarrier, threadAffinity));
	}

	for (auto &thread : m_threads)
	{
		// Set the stealers for each worker thread
		std::vector<Stealer<TaskOperation *, WORKER_QUEUE_SIZE>> stealers;
		for (auto &otherThread : m_threads)
		{
			if (otherThread.get() != thread.get())
			{
				stealers.emplace_back(otherThread->MakeStealer());
			}
		}
		thread->SetStealers(std::move(stealers));
	}

	// Wait for all threads to be created
	m_threadsBarrier.arrive_and_wait();
}

Hush::Threading::Executors::ThreadPool::~ThreadPool()
{
	for (auto &thread : m_threads)
	{
		// Request the threads to stop
		thread->Stop();
		thread->Wake();
		thread->Join();
	}
}

Hush::Threading::Executors::TaskOperation Hush::Threading::Executors::ThreadPool::Schedule()
{
	return TaskOperation(this);
}

void Hush::Threading::Executors::ThreadPool::PushWork(TaskOperation *taskOperation)
{
	{
		std::lock_guard lock(m_globalTasksMutex);
		m_globalTasks.push_back(taskOperation);
	}

	// Notify all worker threads that there is work to do
	for (auto &thread : m_threads)
	{
		thread->Wake();
	}
}
void Hush::Threading::Executors::ThreadPool::NotifyWorkers()
{
	for (auto &thread : m_threads)
	{
		thread->Wake();
	}
}

Hush::Threading::Executors::ThreadPool Hush::Threading::Executors::ThreadPool::Create(ThreadPoolOptions options)
{
	if (options.numThreads == 0)
	{
		options.numThreads = std::thread::hardware_concurrency();
	}

	return {options};
}
