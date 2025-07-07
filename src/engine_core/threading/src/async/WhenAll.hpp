/*! \file WhenAll.hpp
	\author Alan Ramirez
	\date 2025-07-06
	\brief WhenAll coroutine implementation
*/

#pragma once

#include "Assertions.hpp"
#include "TaskTraits.hpp"

#include <atomic>
#include <vector>

namespace Hush::Threading
{
	namespace impl
	{
		class WhenAllLatch
		{
		public:
			WhenAllLatch(size_t count) noexcept
				: m_count(count + 1)
			{
			}

			WhenAllLatch(const WhenAllLatch &) = delete;
			WhenAllLatch(WhenAllLatch &&rhs) noexcept
				: m_count(rhs.m_count.load(std::memory_order::acquire)),
				  m_handle(std::exchange(rhs.m_handle, nullptr))
			{
			}

			WhenAllLatch &operator=(const WhenAllLatch &) = delete;
			WhenAllLatch &operator=(WhenAllLatch &&rhs) noexcept
			{
				if (this != std::addressof(rhs))
				{
					m_count.store(rhs.m_count.load(std::memory_order::acquire), std::memory_order::relaxed);
					m_handle = std::exchange(rhs.m_handle, nullptr);
				}
				return *this;
			}

			bool IsReady() const noexcept
			{
				return m_handle != nullptr && m_handle.done();
			}

			bool TryAwait(std::coroutine_handle<> handle) noexcept
			{
				m_handle = handle;
				return m_count.fetch_sub(1, std::memory_order::acq_rel) > 1;
			}

			void NotifyCompleted() noexcept
			{
				if (m_count.fetch_sub(1, std::memory_order::acq_rel) == 1)
				{
					HUSH_ASSERT(m_handle != nullptr,
								"WhenAllLatch: NotifyCompleted called without a coroutine handle.");
					m_handle.resume();
				}
			}

		private:
			std::atomic<size_t> m_count;

			std::coroutine_handle<> m_handle;
		};

		template <typename TaskContainerType>
		class WhenAllReadyAwaitable;

		template <typename T>
		class WhenAllTask;

		template <>
		class WhenAllReadyAwaitable<std::tuple<>>
		{
		public:
			constexpr WhenAllReadyAwaitable() noexcept = default;
			explicit constexpr WhenAllReadyAwaitable(std::tuple<>) noexcept
			{
			}

			constexpr bool await_ready() const noexcept
			{
				return true; // No tasks to wait for, immediately ready
			}

			void await_suspend(std::coroutine_handle<> handle) noexcept
			{
				(void)handle;
			}

			std::tuple<> await_resume() const noexcept
			{
				return {};
			}
		};

		template <typename... TaskTypes>
		class WhenAllReadyAwaitable<std::tuple<TaskTypes...>>
		{
		public:
			explicit WhenAllReadyAwaitable(TaskTypes &&...tasks) noexcept(
				std::conjunction_v<std::is_nothrow_move_constructible<TaskTypes>...>)
				: m_latch(sizeof...(TaskTypes)),
				  m_tasks(std::move(tasks)...)
			{
			}

			WhenAllReadyAwaitable(const WhenAllReadyAwaitable &) = delete;
			WhenAllReadyAwaitable(WhenAllReadyAwaitable &&rhs) noexcept
				: m_latch(std::move(rhs.m_latch)),
				  m_tasks(std::move(rhs.m_tasks))
			{
			}

			WhenAllReadyAwaitable &operator=(const WhenAllReadyAwaitable &) = delete;
			WhenAllReadyAwaitable &operator=(WhenAllReadyAwaitable &&) noexcept = delete;

			auto operator co_await() & noexcept
			{
				struct Awaiter
				{
					explicit Awaiter(WhenAllReadyAwaitable &awaitable) noexcept
						: m_awaitable(awaitable)
					{
					}

					bool await_ready() const noexcept
					{
						return m_awaitable.m_latch.IsReady();
					}

					bool await_suspend(std::coroutine_handle<> handle) noexcept
					{
						return m_awaitable.m_latch.TryAwait(handle);
					}

					std::tuple<TaskTypes...> &await_resume() const noexcept
					{
						return m_awaitable.m_tasks;
					}

				private:
					WhenAllReadyAwaitable &m_awaitable;
				};

				return Awaiter{*this};
			}

			auto operator co_await() && noexcept
			{
				struct Awaiter
				{
					explicit Awaiter(WhenAllReadyAwaitable &awaitable) noexcept
						: m_awaitable(awaitable)
					{
					}

					bool await_ready() const noexcept
					{
						return m_awaitable.m_latch.IsReady();
					}

					bool await_suspend(std::coroutine_handle<> handle) noexcept
					{
						return m_awaitable.m_latch.TryAwait(handle);
					}

					std::tuple<TaskTypes...> &await_resume() const noexcept
					{
						return m_awaitable.m_tasks;
					}

				private:
					WhenAllReadyAwaitable &m_awaitable;
				};

				return Awaiter{*this};
			}

		private:
			bool IsReady() const noexcept
			{
				return m_latch.IsReady();
			}

			bool TryAwait(std::coroutine_handle<> handle) noexcept
			{
				std::apply([this](auto &&...tasks) { ((tasks.start(m_latch)), ...); }, m_tasks);

				return m_latch.TryAwait(handle);
			}

		private:
			WhenAllLatch m_latch;
			std::tuple<TaskTypes...> m_tasks;
		};

		template <typename TaskContainerType>
		class WhenAllReadyAwaitable
		{
		public:
			explicit WhenAllReadyAwaitable(TaskContainerType &&tasks) noexcept
				: m_latch(std::size(tasks)),
				  m_tasks(std::forward<TaskContainerType>(tasks))
			{
			}
			WhenAllReadyAwaitable(const WhenAllReadyAwaitable &) = delete;
			WhenAllReadyAwaitable(WhenAllReadyAwaitable &&rhs) noexcept
				: m_latch(std::move(rhs.m_latch)),
				  m_tasks(std::move(rhs.m_tasks))
			{
			}

			WhenAllReadyAwaitable &operator=(const WhenAllReadyAwaitable &) = delete;
			WhenAllReadyAwaitable &operator=(WhenAllReadyAwaitable &&) noexcept = delete;

			auto operator co_await() & noexcept
			{
				struct Awaiter
				{
					Awaiter(WhenAllReadyAwaitable &awaitable) noexcept
						: m_awaitable(awaitable)
					{
					}

					bool await_ready() const noexcept
					{
						return m_awaitable.IsReady();
					}

					bool await_suspend(std::coroutine_handle<> handle) noexcept
					{
						return m_awaitable.TryAwait(handle);
					}

					TaskContainerType &await_resume() const noexcept
					{
						return m_awaitable.m_tasks;
					}

				private:
					WhenAllReadyAwaitable &m_awaitable;
				};

				return Awaiter{*this};
			}

			auto operator co_await() && noexcept
			{
				struct Awaiter
				{
					Awaiter(WhenAllReadyAwaitable &awaitable) noexcept
						: m_awaitable(awaitable)
					{
					}

					bool await_ready() const noexcept
					{
						return m_awaitable.IsReady();
					}

					bool await_suspend(std::coroutine_handle<> handle) noexcept
					{
						return m_awaitable.TryAwait(handle);
					}

					TaskContainerType &await_resume() const noexcept
					{
						return m_awaitable.m_tasks;
					}

				private:
					WhenAllReadyAwaitable &m_awaitable;
				};

				return Awaiter{*this};
			}

		private:
			bool IsReady() const noexcept
			{
				return m_latch.IsReady();
			}

			bool TryAwait(std::coroutine_handle<> handle) noexcept
			{
				for (auto &task : m_tasks)
				{
					task.Start(m_latch);
				}

				return m_latch.TryAwait(handle);
			}

		private:
			WhenAllLatch m_latch;
			TaskContainerType m_tasks;
		};

		template <typename T>
		class WhenAllTasksPromise
		{
		public:
			using coroutine_handle_type = std::coroutine_handle<WhenAllTasksPromise<T>>;

			WhenAllTasksPromise() = default;

			auto get_return_object() noexcept
			{
				return coroutine_handle_type::from_promise(*this);
			}

			std::suspend_always initial_suspend() const noexcept
			{
				return {};
			}

			auto final_suspend() noexcept
			{
				struct CompletionNotifier
				{
					bool await_ready() const noexcept
					{
						return false; // Always suspend until notified
					}

					void await_suspend(coroutine_handle_type handle) const noexcept
					{
						handle.promise().m_latch->NotifyAwaitableCompleted();
					}

					void await_resume() const noexcept
					{
					}
				};

				return CompletionNotifier{};
			}

			void unhandle_exception() noexcept
			{
				m_exception = std::current_exception();
			}

			auto yield_value(T &&value) noexcept
			{
				m_value = std::move(value);
				return final_suspend();
			}

			void Start(WhenAllLatch &latch) noexcept
			{
				m_latch = &latch;
				coroutine_handle_type::from_promise(*this).resume();
			}

			T &Result() &
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
				return *m_value;
			}

			T &&Result() &&
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
				return std::forward(*m_value);
			}

			void return_void() noexcept
			{
				// No-op for void return type
			}

		private:
			WhenAllLatch *m_latch = nullptr;
			std::exception_ptr m_exception = nullptr;
			std::add_pointer_t<T> m_value = nullptr;
		};

		template <>
		class WhenAllTasksPromise<void>
		{
		public:
			using coroutine_handle_type = std::coroutine_handle<WhenAllTasksPromise<void>>;
			WhenAllTasksPromise() = default;

			auto get_return_object() noexcept
			{
				return coroutine_handle_type::from_promise(*this);
			}

			std::suspend_always initial_suspend() const noexcept
			{
				return {};
			}

			auto final_suspend() noexcept
			{
				struct CompletionNotifier
				{
					bool await_ready() const noexcept
					{
						return false; // Always suspend until notified
					}

					void await_suspend(coroutine_handle_type handle) const noexcept
					{
						handle.promise().m_latch->NotifyCompleted();
					}

					void await_resume() const noexcept
					{
					}
				};

				return CompletionNotifier{};
			}

			void unhandled_exception() noexcept
			{
				m_exception = std::current_exception();
			}

			void return_void() noexcept
			{
			}

			void Result()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
			}

			void Start(WhenAllLatch &latch)
			{
				m_latch = &latch;
				coroutine_handle_type::from_promise(*this).resume();
			}

		private:
			WhenAllLatch *m_latch = nullptr;
			std::exception_ptr m_exception = nullptr;
		};

		template <typename T>
		class WhenAllTask
		{
		public:
			template <typename TaskContainerType>
			friend class WhenAllReadyAwaitable;

			using promise_type = WhenAllTasksPromise<T>;
			using coroutine_handle_type = typename promise_type::coroutine_handle_type;

			WhenAllTask(coroutine_handle_type handle) noexcept
				: m_handle(handle)
			{
			}

			WhenAllTask(const WhenAllTask &) = delete;
			WhenAllTask(WhenAllTask &&rhs) noexcept
				: m_handle(std::exchange(rhs.m_handle, coroutine_handle_type{}))
			{
			}

			WhenAllTask &operator=(const WhenAllTask &) = delete;
			WhenAllTask &operator=(WhenAllTask &&rhs) noexcept
			{
				if (this != std::addressof(rhs))
				{
					m_handle = std::exchange(rhs.m_handle, coroutine_handle_type{});
				}
				return *this;
			}

			~WhenAllTask() noexcept
			{
				if (m_handle)
				{
					m_handle.destroy();
				}
			}

			decltype(auto) return_value() &
			{
				if constexpr (std::is_void_v<T>)
				{
					m_handle.promise().Result();
				}
				else
				{
					return m_handle.promise().Result();
				}
			}

			decltype(auto) return_value() &&
			{
				if constexpr (std::is_void_v<T>)
				{
					m_handle.promise().Result();
					return;
				}
				else
				{
					return m_handle.promise().Result();
				}
			}

		private:
			void Start(impl::WhenAllLatch &latch) noexcept
			{
				m_handle.promise().Start(latch);
			}

		private:
			coroutine_handle_type m_handle;
		};

		template <Concepts::Awaitable A, typename T = typename Concepts::AwaitableTraits<A>::ResultType>
		static WhenAllTask<T> MakeWhenAllTask(A awaitable);

		template <Concepts::Awaitable A, typename T>
		static WhenAllTask<T> MakeWhenAllTask(A awaitable)
		{
			if constexpr (std::is_void_v<T>)
			{
				co_await std::forward<A>(awaitable);
				co_return;
			}
			else
			{
				co_yield co_await std::move(awaitable);
			}
		}
	} // namespace impl

	template <Concepts::Awaitable... Awaitables>
	[[nodiscard]]
	auto WhenAll(Awaitables &&...awaitables)
	{
		return impl::WhenAllReadyAwaitable<
			std::tuple<impl::WhenAllTask<typename Concepts::AwaitableTraits<Awaitables>::ResultType>...>>(
			std::make_tuple(impl::MakeWhenAllTask(std::move(awaitables)...)));
	}

	template <std::ranges::range RangeType,
			  Concepts::Awaitable AwaitableType = typename std::ranges::range_value_t<RangeType>,
			  typename T = typename Concepts::AwaitableTraits<AwaitableType>::ResultType>
	[[nodiscard]]
	auto WhenAll(RangeType awaitables)
	{
		std::vector<impl::WhenAllTask<T>> tasks;

		if constexpr (std::ranges::sized_range<RangeType>)
		{
			tasks.reserve(std::ranges::size(awaitables));
		}

		// Next, we need to wrap each awaitable in a WhenAllTask
		for (auto &&awaitable : awaitables)
		{
			tasks.emplace_back(impl::MakeWhenAllTask(std::move(awaitable)));
		}

		return impl::WhenAllReadyAwaitable<std::vector<impl::WhenAllTask<T>>>(std::move(tasks));
	}

} // namespace Hush::Threading