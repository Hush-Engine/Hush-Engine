/*! \file Result.hpp
    \author Alan Ramirez Herrera
    \date 2024-03-07
    \brief typedefs for nonstd::expected
*/

#pragma once

#include <expected.hpp>
#include <string>

namespace Hush
{
    template <typename T, typename E> using Result = nonstd::expected<T, E>;

    template <typename E> auto Err(E &&arg)
    {
        return nonstd::make_unexpected(std::forward<E>(arg));
    }
} // namespace Hush