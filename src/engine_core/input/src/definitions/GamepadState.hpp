#pragma once
#include "Vector3Math.hpp"
#include "definitions/GamepadButtons.hpp"
#include "definitions/KeyStates.hpp"
#include <array>
#include <cstdint>
namespace Hush
{
	struct GamepadButtonState
	{
		EGamepadButton_t button = (EGamepadButton_t)EGamepadButton::Invalid;
		EKeyState_t currentState = (EKeyState_t)EKeyState::None;
		EKeyState_t previousState = (EKeyState_t)EKeyState::None;
	};

	struct GamepadState
	{
		std::array<GamepadButtonState, static_cast<size_t>(EGamepadButton::Max)> buttons{};
		std::array<float, static_cast<size_t>(EGamepadAxis::Max)> axis{};
		/// @brief A reference to its SDLGamepad open pointer
		void *sdlHandle = nullptr;
		/// @brief The Id through which SDL identifies this gamepad
		uint32_t internalId = 0;
		// Buttons are handled on a per-call level(?
	};

} // namespace Hush
