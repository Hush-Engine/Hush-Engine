/*! \file Executor.hpp
	\author Alan Ramirez
	\date 2025-01-10
	\brief Executor concept
*/

#pragma once
#include "TaskTraits.hpp"
#include "SelfDeleteTask.hpp"

namespace Hush::Threading
{
	namespace Concepts
	{
		///
		/// Concept executor, if T is a pointer, it should be dereferenceable and have a Schedule method.
		/// If T is not a pointer, it should have a Schedule method.
		template <typename T>
		concept Executor = requires(T executor) {
			{ executor.Schedule() };
		} || requires(T *executor) {
			{ executor->Schedule() };
		};

		template <typename T, typename = void>
		struct ExecutorTraits
		{
		};

		template <typename T>
		struct ExecutorTraits<T>
		{
			static constexpr bool is_executor = Executor<T>;
			using ReturnType = std::invoke_result_t<decltype(&T::Schedule), T>;
		};

	} // namespace Concepts

	template <typename T>
	[[nodiscard]]
	Task<void> RunOn(Hush::Threading::Concepts::Executor auto *executor, Task<T> task)
	{
		co_await executor->Schedule();
		co_await task;
	}

	template <typename T>
	[[nodiscard]]
	void SpawnOn(Hush::Threading::Concepts::Executor auto *executor, Task<T> task)
	{
		auto wrapperTask = [](Hush::Threading::Concepts::Executor auto *executor, Task<T> t) -> Task<void> {
			co_await executor->Schedule();
			co_await t;
		}(executor, std::move(task));
		auto selfDeleteTask = MakeSelfDeleteTask(std::move(wrapperTask));

		// Force first run.
		selfDeleteTask.Resume();
	}

} // namespace Hush::Threading