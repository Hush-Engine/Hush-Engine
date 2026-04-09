/*! \file KeyStates.hpp
	\author Kyn21kx
	\date 2024-02-28
	\brief Represents the possible states of a key on an input event
*/

#pragma once

namespace Hush
{
	enum class [[hush::export]] EKeyState
	{
		None = -1,
		Pressed,
		Held,
		Released
	};
} // namespace Hush
