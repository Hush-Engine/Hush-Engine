/*! \file KeyStates.hpp
	\author Kyn21kx
	\date 2024-02-28
	\brief Represents the possible states of a key on an input event
*/

#pragma once

#include <cstdint>
namespace Hush
{
	enum class [[hush::export]] EKeyState
	{
		None = -1,
		Pressed,
		Held,
		Released
	};

	// HACK: Do not use inheritance on the enum but this alias for its sizing due to limitations of the hush::export
	// tool
	using EKeyState_t = int8_t;
} // namespace Hush
