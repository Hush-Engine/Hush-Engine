/*! \file SelfDeleteTask.hpp
	\author Alan Ramirez
	\date 2025-07-17
	\brief Self delete task
*/

#pragma once

// NOLINTBEGIN(readability-identifier-naming, modernize-use-nodiscard)

#include "Task.hpp"

namespace Hush::Threading
{
	namespace impl
	{
		class SelfDeleteTask;

		class SelfDeletePromise
		{
		public:
			SelfDeletePromise() noexcept = default;

			SelfDeletePromise(SelfDeletePromise &&) noexcept = default;
			SelfDeletePromise(const SelfDeletePromise &) = delete;
			SelfDeletePromise &operator=(SelfDeletePromise &&) noexcept = default;
			SelfDeletePromise &operator=(const SelfDeletePromise &) = delete;
			~SelfDeletePromise() noexcept = default;

			SelfDeleteTask get_return_object() noexcept;

			std::suspend_always initial_suspend() noexcept
			{
				return {};
			}

			std::suspend_never final_suspend() noexcept
			{
				return {};
			}

			void return_void() noexcept
			{
			}

			void unhandled_exception() noexcept
			{
				// We ignore them, we don't use exceptions in Hush nor we care about them in a self delete task.
			}
		};

		class SelfDeleteTask
		{
		public:
			using promise_type = SelfDeletePromise;

			explicit SelfDeleteTask(promise_type &promise) noexcept
				: m_promise(&promise)
			{
			}
			SelfDeleteTask(SelfDeleteTask &&other) noexcept
				: m_promise(std::exchange(other.m_promise, nullptr))
			{
			}

			SelfDeleteTask &operator=(SelfDeleteTask &&other) noexcept
			{
				if (this != &other)
				{
					m_promise = std::exchange(other.m_promise, nullptr);
				}
				return *this;
			}
			SelfDeleteTask(const SelfDeleteTask &) = delete;
			SelfDeleteTask &operator=(const SelfDeleteTask &) = delete;

			[[nodiscard]]
			promise_type *GetPromise() const noexcept
			{
				return m_promise;
			}

			[[nodiscard]]
			std::coroutine_handle<promise_type> GetCoroutineHandle() const noexcept
			{
				return std::coroutine_handle<promise_type>::from_promise(*m_promise);
			}

			bool Resume() const noexcept
			{
				if (m_promise != nullptr)
				{
					auto handle = GetCoroutineHandle();
					if (!handle.done())
					{
						handle.resume();
						return true;
					}
				}
				return false;
			}

		private:
			promise_type *m_promise;
		};

		inline SelfDeleteTask SelfDeletePromise::get_return_object() noexcept
		{
			return SelfDeleteTask(*this);
		}
	} // namespace impl

	inline impl::SelfDeleteTask MakeSelfDeleteTask(Task<void> task) noexcept
	{
		co_await task;
		co_return;
	}
} // namespace Hush::Threading

// NOLINTEND(readability-identifier-naming, modernize-use-nodiscard)
