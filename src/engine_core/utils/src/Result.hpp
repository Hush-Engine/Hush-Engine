/*! \file Result.hpp
    \author Alan Ramirez
    \date 2024-10-08
    \brief Result wrapper
*/

#pragma once
#include <outcome.hpp>

namespace Hush
{
    template <typename T, typename E>
    using Result = outcome_v2::result<T, E>;
};