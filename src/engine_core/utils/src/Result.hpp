/*! \file Result.hpp
	\author Alan Ramirez
	\date 2024-10-08
	\brief Result wrapper
*/

#pragma once
#include <outcome.hpp>

namespace Hush
{
	// TODO: Discuss about checked / unchecked and NoValuePolicies
	template <typename T, typename E>
	using Result = outcome_v2::unchecked<T, E>;

	inline auto Success()
	{
		return outcome_v2::success();
	}
}; // namespace Hush