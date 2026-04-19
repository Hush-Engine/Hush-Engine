Job System
==========

Hush Engine provides a coroutine-based job system built on C++20 coroutines and a work-stealing
thread pool. It allows you to schedule and execute tasks concurrently without managing threads directly.

.. contents::
    :local:
    :depth: 2

The Engine ThreadPool
---------------------
.. _The Engine ThreadPool:
.. index:: ThreadPool

The engine owns a ``ThreadPool`` instance that is created during initialization. It uses
``std::thread::hardware_concurrency()`` threads, each pinned to a CPU core. You should use this
pool for all your concurrent work rather than creating your own.

Access it through the engine pointer:

.. code-block:: cpp

    Hush::Threading::Executors::ThreadPool *pool = engine->GetEngineThreadPool();

The pool is passed to subsystems like ``Scene`` automatically. If you're writing a system that
receives the engine pointer, use ``GetEngineThreadPool()`` to obtain the pool.

Quick Start
-----------
.. _Quick Start:
.. index:: Quick Start

Here is a minimal example that schedules a task on the engine's thread pool:

.. code-block:: cpp

    #include <async/Task.hpp>
    #include <async/Executor.hpp>
    #include <async/SyncWait.hpp>

    using namespace Hush::Threading;
    using namespace Hush::Threading::Executors;

    // Define a coroutine that returns Task<void>
    auto myTask = [](bool &result) -> Task<void> {
        result = true;
        co_return;
    };

    // Get the engine's thread pool
    ThreadPool *pool = engine->GetEngineThreadPool();

    bool done = false;

    // Schedule the task on the pool
    Task<void> task = RunOn(pool, myTask(done));

    // Block until the task completes
    Wait(task);

    // done is now true

Tasks
-----
.. _Tasks:
.. index:: Tasks

``Task<T>`` is the coroutine return type used throughout the job system. It is a move-only type
that represents a deferred computation.

- ``Task<void>`` -- a task that produces no result.
- ``Task<int>`` -- a task that produces an ``int`` value.
- ``Task<T&>`` -- a task that produces a reference.

Create a task by writing a coroutine function that returns ``Task<T>``:

.. code-block:: cpp

    Task<int> ComputeValue(int input) {
        co_return input * 2;
    }

    Task<void> DoWork() {
        int result = co_await ComputeValue(21);
        // result == 42
        co_return;
    }

You can check if a task has completed with ``Ready()``:

.. code-block:: cpp

    Task<void> task = RunOn(pool, DoWork());
    // task.Ready() returns true once the coroutine has finished

Scheduling Tasks
----------------
.. _Scheduling Tasks:
.. index:: Scheduling

RunOn
^^^^^

``RunOn`` schedules a task on an executor (e.g., the thread pool) and returns a ``Task<void>``
that can be awaited or passed to ``Wait()``:

.. code-block:: cpp

    auto taskFunc = []() -> Task<void> {
        // This runs on a thread pool worker
        co_return;
    };

    Task<void> task = RunOn(pool, taskFunc());

    // Either await it from another coroutine:
    co_await task;

    // Or block from non-coroutine code:
    Wait(task);

SpawnOn
^^^^^^^

``SpawnOn`` is the fire-and-forget variant. The task will execute on the pool but you cannot
await its completion or retrieve its result:

.. code-block:: cpp

    auto logTask = [](std::string_view msg) -> Task<void> {
        Hush::LogInfo(msg);
        co_return;
    };

    SpawnOn(pool, logTask("Background work done"));

Use ``SpawnOn`` for non-critical side effects like logging or telemetry.

Synchronization
---------------
.. _Synchronization:
.. index:: Synchronization

Wait
^^^^

``Wait`` synchronously blocks the calling thread until an awaitable completes. Use it to bridge
between non-coroutine code (e.g., ``main()``) and the coroutine world:

.. code-block:: cpp

    Task<void> task = RunOn(pool, DoWork());
    Wait(task); // Blocks until DoWork() finishes

.. warning::

    Do not call ``Wait()`` from inside a coroutine. This will block a worker thread and may
    cause a deadlock. Use ``co_await`` instead.

WhenAll
^^^^^^^

``WhenAll`` waits for multiple tasks to complete. It supports both variadic and range-based usage:

.. code-block:: cpp

    // Range-based: pass a vector of tasks
    std::vector<Task<void>> tasks;
    for (int i = 0; i < 4; ++i) {
        tasks.push_back(RunOn(pool, taskFunc()));
    }
    Wait(WhenAll(std::move(tasks)));

Since tasks run in parallel, the total time is approximately the duration of the longest task,
not the sum of all tasks.

ParallelFor
-----------
.. _ParallelFor:
.. index:: ParallelFor

``ParallelFor`` splits an iterator range into chunks and processes them in parallel. It handles
partitioning automatically:

- Maximum 1024 tasks to avoid overwhelming the pool.
- Minimum 1000 elements per chunk.

.. code-block:: cpp

    #include <utils/ParallelUtils.hpp>

    std::vector<int> data(10000);
    for (int i = 0; i < 10000; ++i) {
        data[i] = i;
    }

    // Increment every element in parallel
    Wait(ParallelFor(pool, data.begin(), data.end(), [](int &value) {
        value += 1;
    }));

    // data[0] == 1, data[1] == 2, ...

``ParallelFor`` returns a ``Task<void>`` that must be awaited or passed to ``Wait()``.

Best Practices
--------------
.. _Best Practices:
.. index:: Best Practices

- **Use the engine's thread pool.** Don't create your own ``ThreadPool`` unless you have a
  specific reason to isolate work from the rest of the engine.
- **Use ``co_await`` inside coroutines, ``Wait()`` outside.** Calling ``Wait()`` inside a
  coroutine blocks a worker thread and risks deadlock.
- **Prefer ``ParallelFor`` for data-parallel work** over manually spawning tasks with a loop.
- **Use ``WhenAll`` to wait for multiple tasks** rather than calling ``Wait()`` on each one
  sequentially.
- **Keep tasks independent.** Avoid shared mutable state between tasks. If you must share state,
  use proper synchronization (e.g., ``std::mutex``).

ThreadPool (Advanced)
---------------------
.. _ThreadPool Advanced:
.. index:: ThreadPool

For advanced use cases where you need a separate pool (e.g., isolating I/O-bound work), you can
create one manually:

.. code-block:: cpp

    #include <executors/ThreadPool.hpp>

    using namespace Hush::Threading::Executors;

    ThreadPoolOptions options;
    options.numThreads = 4;
    options.pinToCore = false;

    ThreadPool pool = ThreadPool::Create(options);

    // Use it like the engine pool
    Wait(RunOn(&pool, myTask()));

``ThreadPoolOptions`` fields:

- ``numThreads`` -- Number of worker threads. Defaults to ``std::thread::hardware_concurrency()``.
- ``pinToCore`` -- Whether to pin each thread to a CPU core. Defaults to ``false``.

``ThreadPool`` is non-copyable and non-movable. It must outlive all tasks scheduled on it.

API Reference
-------------
.. _API Reference:
.. index:: API Reference

.. doxygenstruct:: Hush::Threading::Executors::ThreadPoolOptions
    :members:

.. doxygenclass:: Hush::Threading::Executors::ThreadPool
    :members:
