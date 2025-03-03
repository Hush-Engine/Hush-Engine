/*! \file ThreadPool.cpp
    \author Alan Ramirez
    \date 2024-12-25
    \brief ThreadPool implementation
*/
#include "ThreadPool.hpp"
#include "Platform.hpp"
#include "async/Task.hpp"

#include <Logger.hpp>
#include <mutex>
#include <random>
#include <semaphore>
#include <thread>

Hush::Threading::impl::WorkerQueue::WorkerQueue(ThreadPool *threadPool)
    : m_threadPool(threadPool),
      m_tasks(),
      m_bottom(0),
      m_top(0)
{
}

Hush::Threading::impl::WorkerQueue::WorkerQueue(WorkerQueue &&other) noexcept
    : m_threadPool(other.m_threadPool),
      m_bottom(other.m_bottom.load(std::memory_order_acquire)),
      m_top(other.m_top.load(std::memory_order_acquire)),
      m_tasks(std::move(other.m_tasks))
{
    // We use release memory order since we are moving the tasks to another queue.
    other.m_bottom.store(0, std::memory_order_release);
    other.m_top.store(0, std::memory_order_release);
}

Hush::Threading::impl::WorkerQueue &Hush::Threading::impl::WorkerQueue::operator=(WorkerQueue &&other) noexcept
{
    if (this == &other)
    {
        return *this;
    }

    this->m_threadPool = std::move(other.m_threadPool);
    this->m_bottom = other.m_bottom.load(std::memory_order_acquire);
    this->m_top = other.m_top.load(std::memory_order_acquire);
    this->m_tasks = std::move(other.m_tasks);

    other.m_bottom.store(0, std::memory_order_release);
    other.m_top.store(0, std::memory_order_release);
    other.m_threadPool = nullptr;

    return *this;
}

void Hush::Threading::impl::WorkerQueue::Push(TaskOperation *task)
{
    // Push a task to the bottom of the queue
    std::int64_t bottom = m_bottom.load(std::memory_order_acquire);
    std::int64_t newBottom = bottom + 1;

    // if (newBottom >= WORKER_QUEUE_SIZE)
    // {
    //     // The queue is full
    //     // Push the task to the global queue
    //     m_threadPool->PushToGlobalQueue(task);
    //     return;
    // }

    m_tasks[bottom % WORKER_QUEUE_SIZE] = task;

    // Ok, just update the bottom
    m_bottom.store(newBottom, std::memory_order_release);

    // Check if the queue was full
    std::int64_t top = m_top.load(std::memory_order_acquire);
    if (top == bottom)
    {
        // The queue was empty
        m_threadPool->NotifyWorkerThreads();
    }
}

Hush::Threading::TaskOperation *Hush::Threading::impl::WorkerQueue::Pop()
{
    // Pops a task from the bottom of the queue.
    m_bottom.fetch_sub(1, std::memory_order_acquire);
    std::int64_t bottom = m_bottom.load(std::memory_order_acquire);
    std::int64_t top = m_top.load(std::memory_order_acquire);

    if (top <= bottom)
    {
        // Non-empty queue, just pop the task
        TaskOperation *task = m_tasks[bottom % WORKER_QUEUE_SIZE];

        if (top != bottom)
        {
            // There are more tasks in the queue.
            return task;
        }

        // Last task in the queue
        if (!m_top.compare_exchange_weak(top, top + 1, std::memory_order_acquire, std::memory_order_relaxed))
        {
            // Another thread stole the task
            return nullptr;
        }

        m_bottom.store(top + 1, std::memory_order_release);

        return task;
    }
    // The queue is empty
    m_bottom.store(top, std::memory_order_relaxed);
    return nullptr;
}

Hush::Threading::TaskOperation *Hush::Threading::impl::WorkerQueue::Steal()
{
    // Steals a task from the top of the queue.
    std::int64_t top = m_top.load(std::memory_order_acquire);
    std::int64_t bottom = m_bottom.load(std::memory_order_acquire);

    // Check if the queue is empty
    if (top >= bottom)
    {
        return nullptr;
    }

    // Non-empty queue, just steal the task
    TaskOperation *task = m_tasks[top % WORKER_QUEUE_SIZE];

    if (!m_top.compare_exchange_strong(top, top + 1))
    {
        // Another thread stole the task
        return nullptr;
    }

    // Task stolen
    return task;
}

std::size_t Hush::Threading::impl::WorkerQueue::TakeFromGlobalQueue()
{
    std::array<TaskOperation *, WorkerThread::DEFAULT_STEAL_COUNT> tasks;

    const auto tasksAcquired = m_threadPool->StealFromGlobalQueue(tasks);

    for (std::size_t i = 0; i < tasksAcquired; ++i)
    {
        Push(tasks[i]);
    }

    return tasksAcquired;
}

Hush::Threading::TaskOperation *Hush::Threading::impl::WorkerQueue::StealFromOtherThread(std::uint32_t threadNumber)
{
    return m_threadPool->StealFromOtherThread(threadNumber);
}

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

Hush::Threading::impl::WorkerThread::WorkerThread(std::unique_ptr<WorkerQueue> workerQueue, std::uint32_t threadIndex,
                                                  ThreadOptions options) noexcept
    : m_workerQueue(std::move(workerQueue)),
      m_threadIndex(threadIndex),
      m_options(options)
{
    m_state.store(EWorkerThreadState::None, std::memory_order_relaxed);

    m_thread = std::jthread([this](std::stop_token stopToken) {
        if (m_options.affinity >= 0)
        {
            SetCurrentThreadAffinity(m_options.affinity);
        }

        // Wait until the thread is started
        m_state.wait(EWorkerThreadState::None, std::memory_order_acquire);

        // Start the thread function
        this->ThreadFunction(stopToken);
    });
}

Hush::Threading::impl::WorkerThread::~WorkerThread()
{
}

void Hush::Threading::impl::WorkerThread::Start()
{
    // Notify the thread to start with the condition variable
    m_state.store(EWorkerThreadState::Starting, std::memory_order_release);
    m_state.notify_all();
}

void Hush::Threading::impl::WorkerThread::Stop(EStopMode stopMode)
{
    m_stopMode = stopMode;

    // Stop the thread
    m_thread.request_stop();

    // Ensure to wake up the thread
    m_state.store(EWorkerThreadState::Stopping, std::memory_order_release);
    m_state.notify_all();
}

void Hush::Threading::impl::WorkerThread::Notify()
{
    m_state.store(EWorkerThreadState::Running, std::memory_order_release);
    m_state.notify_all();
}

void Hush::Threading::impl::WorkerThread::ThreadFunction(std::stop_token stopToken)
{
    m_state = EWorkerThreadState::Running;

    while (!stopToken.stop_requested())
    {
        TaskOperation *task = m_workerQueue->Pop();

        // Unfortunatelly, we need to check if we are stopping after each step as it might be that the thread was
        // stopped
        if (m_state.load(std::memory_order_acquire) == EWorkerThreadState::Stopping)
        {
            break;
        }

        if (task == nullptr)
        {
            // Steal a task
            /*
             * TODO: fix this, stealing causes some problems due to atomic operations
             * it might be that the wait-free algorithm is not correct (which is likely the case)
             * However, I would suggest looking at other implementation or using an existing library.
             * The use case is as follows:
             * 1. There must be a global queue, it does not need to be lock-free, but it should be thread-safe.
             *    This queue is used to store tasks pushed by any thread that **does not** belong to the thread pool.
             * 2. Each thread has its own queue, this queue should be lock-free (ideally wait-free) and should be used
             *    to store tasks that either were taken from the global queue, pushed by the thread itself, or stolen
             * from other threads. It is possible that we implement two queues, one that is not thread-safe and is used
             * to store tasks that cannot be stolen by other threads, and another that is thread-safe and is used to
             * store tasks that can be stolen by other threads, but it is an implementation detail that we should
             * discuss.
             * 3. The stealing algorithm **should** be wait-free, it should take a task from the top of the queue
             *    of any random thread, if the queue is empty, it should try to steal from another thread, and so on.
             * 4. When a thread is Idle, it should be awaken by another thread that has a task to execute. This should
             * be as follows: the main thread should check if all threads are idle, if so, it should wake up one of the
             *    idle threads. The idle thread will take a batch of tasks from the global queue and push them to its
             * own queue. If the amount of tasks to run is more than a given heuristic, the thread should wake up
             * another idle thread, and so on.
             * 5. When an idle thread is awaken, it should check if there are tasks in the global queue, if so, it
             * should take a batch of tasks and push them to its own queue. Repeat number 4.
             * 6. If an awaken thread could not find any tasks in the global queue, it should steal from another thread
             * (see 3).
             *
             * I think that we should look into existing implementations of lock-free executor services,
             * for instance, the go runtime has a lock-free scheduler, we could look into that. Also Tokio.rs
             * has a lock-free scheduler, and they provide a blog post explaining how it works.
             */
            // task = m_workerQueue->StealFromOtherThread(m_threadIndex);
        }

        // Unfortunately, we need to check if we are stopping after each step as it might be that the thread was stopped
        if (m_state.load(std::memory_order_acquire) == EWorkerThreadState::Stopping)
        {
            break;
        }

        // No task could be stolen, check the global queue
        std::size_t acquiredTasks = 0;
        if (task == nullptr)
        {
            acquiredTasks = m_workerQueue->TakeFromGlobalQueue();
        }

        if (task != nullptr)
        {
            task->m_awaitingCoroutine.resume();

            if (task->m_shouldDeleteWhenDone)
            {
                task->m_awaitingCoroutine.destroy();
            }
        }
        else if (acquiredTasks == 0)
        {
            // No task could be found either in the local, external local, or global queue.
            // Perform some busy waiting
            // TODO: Perform some busy waiting while checking for tasks in the current thread's queue
            // std::this_thread::sleep_for(std::chrono::milliseconds(1));

            // It might be that the thread was stopped after the while loop condition was checked, so we need to check
            // again before going to sleep
            if (m_state.load(std::memory_order_acquire) == EWorkerThreadState::Running)
            {
                // No tasks to execute, put the thread to sleep
                m_state.store(EWorkerThreadState::Idle, std::memory_order_release);
                m_state.wait(EWorkerThreadState::Idle, std::memory_order_acquire);
                // Thread woke up, continue execution
            }
        }
    }

    if (m_stopMode == EStopMode::FinishPendingTasks)
    {
        // Finish pending tasks
        TaskOperation *task = m_workerQueue->Pop();

        while (task != nullptr)
        {
            task->m_awaitingCoroutine.resume();
            task = m_workerQueue->Pop();
        }
    }
    m_state.store(EWorkerThreadState::Stopped, std::memory_order_relaxed);
}

void Hush::Threading::TaskOperation::await_suspend(std::coroutine_handle<Job::promise_type> awaitingCoroutine) noexcept
{
    m_awaitingCoroutine = awaitingCoroutine;
    awaitingCoroutine.promise().SetFlag(m_done);

    m_executor.PushToGlobalQueue(this);

    m_executor.NotifyWorkerThreads();
}

void Hush::Threading::TaskOperation::await_resume() noexcept
{
}

Hush::Threading::ThreadPool::ThreadPool(std::uint32_t numThreads)
{
    const std::uint32_t numHardwareThreads = std::thread::hardware_concurrency();
    // Get hardware threads
    if (numThreads == 0)
    {
        numThreads = std::thread::hardware_concurrency();
    }

    for (std::uint32_t i = 0; i < numThreads; ++i)
    {
        const std::int32_t affinity = numThreads == numHardwareThreads ? static_cast<std::int32_t>(i) : -1;
        auto workerQueue = std::make_unique<impl::WorkerQueue>(this);

        const auto threadOptions = impl::WorkerThread::ThreadOptions{.affinity = affinity};

        m_workerThreads.emplace_back(std::make_unique<impl::WorkerThread>(std::move(workerQueue), i, threadOptions));
    }
}
Hush::Threading::ThreadPool::~ThreadPool()
{
    for (auto &thread : m_workerThreads)
    {
        thread->Stop(impl::WorkerThread::EStopMode::StopImmediately);
    }
}

Hush::Threading::TaskOperation *Hush::Threading::ThreadPool::StealFromOtherThread(std::uint32_t threadNumber)
{
    if ((threadNumber + 1) == m_workerThreads.size())
    {
        // We cannot steal from the current thread
        return nullptr;
    }

    // TODO: improve random number generation
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::uint32_t> dist(0, static_cast<std::uint32_t>(m_workerThreads.size() - 1));

    std::uint32_t randomIndex = dist(gen);

    // First, check if any thread is stopped OR stopping
    for (std::uint32_t i = 0; i < m_workerThreads.size(); ++i)
    {
        if (m_workerThreads[i]->GetState() == impl::WorkerThread::EWorkerThreadState::Stopped ||
            m_workerThreads[i]->GetState() == impl::WorkerThread::EWorkerThreadState::Stopping)
        {
            return nullptr;
        }
    }

    while (randomIndex == threadNumber)
    {
        randomIndex = dist(gen);
    }

    return m_workerThreads[randomIndex]->m_workerQueue->Steal();
}
Hush::Threading::TaskOperation Hush::Threading::ThreadPool::ScheduleCurrentTask()
{
    return TaskOperation(*this);
}

Hush::Threading::Job Hush::Threading::ThreadPool::WrapTask(ThreadPool &threadPool, Task<void> function)
{
    co_await threadPool.ScheduleCurrentTask();
    co_await function;
}

Hush::Threading::Job Hush::Threading::ThreadPool::ScheduleTask(Task<void> &&task)
{
    auto wrapper = WrapTask(*this, std::move(task));

    wrapper.GetCoroutine().resume();

    return std::move(wrapper);
}

void Hush::Threading::ThreadPool::Start()
{
    // Start the threads
    for (auto &thread : m_workerThreads)
    {
        thread->Start();
    }
}

void Hush::Threading::ThreadPool::WaitUntilDone()
{
    // Wait until all threads are Idle AND the global queue is empty
    bool allThreadsIdle = false;
    bool globalQueueEmpty = false;

    while (!allThreadsIdle || !globalQueueEmpty)
    {
        allThreadsIdle = true;

        {
            std::lock_guard lock(m_globalQueueMutex);
            globalQueueEmpty = m_globalQueue.empty();
        }

        if (!globalQueueEmpty)
        {
            for (auto &thread : m_workerThreads)
            {
                // Check if thread is not idle or stopped
                if (thread->GetState() != impl::WorkerThread::EWorkerThreadState::Idle &&
                    thread->GetState() != impl::WorkerThread::EWorkerThreadState::Stopped)
                {
                    allThreadsIdle = false;
                    break;
                }
            }

            // For each idle thread, notify that the global queue is not empty
            for (auto &thread : m_workerThreads)
            {
                if (thread->GetState() == impl::WorkerThread::EWorkerThreadState::Idle)
                {
                    thread->Notify();
                }
            }
        }

        if (!allThreadsIdle || !globalQueueEmpty)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
    }
}

Hush::Threading::TaskOperation *Hush::Threading::ThreadPool::StealFromGlobalQueue()
{
    // Lock the global queue
    std::lock_guard lock(m_globalQueueMutex);

    if (m_globalQueue.empty())
    {
        return nullptr;
    }

    TaskOperation *task = m_globalQueue.back();
    m_globalQueue.pop_back();

    return task;
}

std::size_t Hush::Threading::ThreadPool::StealFromGlobalQueue(std::span<TaskOperation *> tasks)
{
    std::size_t stolenTasks = 0;

    std::lock_guard lock(m_globalQueueMutex);
    for (std::size_t i = 0; i < tasks.size(); ++i)
    {
        if (m_globalQueue.empty())
        {
            break;
        }

        tasks[i] = m_globalQueue.front();
        m_globalQueue.pop_front();
        ++stolenTasks;
    }

    return stolenTasks;
}

void Hush::Threading::ThreadPool::NotifyWorkerThreads()
{
    for (auto &thread : m_workerThreads)
    {
        thread->Notify();
    }
}

void Hush::Threading::ThreadPool::PushToGlobalQueue(TaskOperation *task)
{
    // Lock the global queue
    std::lock_guard lock(m_globalQueueMutex);
    m_globalQueue.push_back(task);
}
