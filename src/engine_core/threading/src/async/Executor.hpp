/*! \file Executor.hpp
    \author Alan Ramirez
    \date 2025-01-10
    \brief Executor concept
*/

#pragma once
#include "TaskTraits.hpp"

namespace Hush::Threading
{
    namespace Concepts
    {
        template <typename T>
        concept Executor = requires(T executor) {
            { executor.Schedule() } -> Awaitable;
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

    ///
    /// @tparam E Executor
    /// @param executor Executor to schedule.
    /// @return
    template <Concepts::Executor E>
    typename Concepts::ExecutorTraits<E>::ReturnType Schedule(E &executor)
    {
        co_await executor.Schedule();
    }

} // namespace Hush::Threading