#include "InputManager.hpp"
#include "Assertions.hpp"
#include "Logger.hpp"
#include "Platform.hpp"
#include "definitions/GamepadButtons.hpp"
#include "definitions/GamepadState.hpp"
#include "definitions/KeyCode.hpp"
#include "definitions/KeyStates.hpp"
#include <SDL3/SDL_gamepad.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <SDL3/SDL_mouse.h>

#define IS_CURRENTLY_PRESSED(key) (key == (EKeyState_t)EKeyState::Pressed || key == (EKeyState_t)EKeyState::Held)

// TODO: Populate the map in the stack with all enums
// NOLINTNEXTLINE
std::vector<Hush::KeyData> Hush::InputManager::S_KEY_DATA_BY_CODE = {};
// NOLINTNEXTLINE
Hush::MouseData Hush::InputManager::S_MOUSE_DATA = {};

std::array<Hush::GamepadState, Hush::InputManager::MAX_GAMEPADS> Hush::InputManager::S_GAMEPAD_STATES = {};

void Hush::InputManager::Init()
{
	S_KEY_DATA_BY_CODE.resize((uint32_t)EKeyCode::SdlNumScancodes);
}

bool Hush::InputManager::IsKeyDown(EKeyCode key)
{
	return IS_CURRENTLY_PRESSED(S_KEY_DATA_BY_CODE[(uint32_t)key].currentState);
}

bool Hush::InputManager::IsKeyDownThisFrame(EKeyCode key)
{
	return S_KEY_DATA_BY_CODE[(uint32_t)key].currentState == (EKeyState_t)EKeyState::Pressed;
}

bool Hush::InputManager::IsKeyUp(EKeyCode key)
{
	return S_KEY_DATA_BY_CODE[(uint32_t)key].currentState == (EKeyState_t)EKeyState::Released;
}

bool Hush::InputManager::IsKeyHeld(EKeyCode key)
{
	return S_KEY_DATA_BY_CODE[(uint32_t)key].currentState == (EKeyState_t)EKeyState::Held;
}

bool Hush::InputManager::GetMouseButtonPressed(EMouseButton button)
{
	return MouseMapContains(button) && IS_CURRENTLY_PRESSED(((EKeyState_t)S_MOUSE_DATA.mouseButtonMap[button]));
}

bool Hush::InputManager::FetchCharThisFrame(char *outChar)
{
	*outChar = s_lastChar;
	return s_lastChar != 0;
}

void Hush::InputManager::ResetCharData()
{
	s_lastChar = 0;
	SendKeyEvent((int)EKeyCode::KpColon, EKeyState::Released);
}

void Hush::InputManager::SendCharEvent(char pressedChar)
{
	s_lastChar = pressedChar;
}

void Hush::InputManager::MarkPressedKeysForNextFrame()
{
	for (auto &ref : S_KEY_DATA_BY_CODE)
	{
		if (ref.currentState == (EKeyState_t)EKeyState::Pressed)
		{
			// Will be marked as held, and it's SDL's job to mark it as released
			ref.currentState = (EKeyState_t)EKeyState::Held;
			ref.previousState = (EKeyState_t)EKeyState::Pressed;
		}
	}

	// Praying for a loop unroll on this one
	for (GamepadState &gamepad : S_GAMEPAD_STATES)
	{
		for (GamepadButtonState &ref : gamepad.buttons)
		{
			if (ref.currentState == (EKeyState_t)EKeyState::Pressed)
			{
				// Will be marked as held, and it's SDL's job to mark it as released
				ref.currentState = (EKeyState_t)EKeyState::Held;
				ref.previousState = (EKeyState_t)EKeyState::Pressed;
			}
		}
	}
}

glm::vec2 Hush::InputManager::GetMousePosition()
{
	return glm::vec2{S_MOUSE_DATA.positionX, S_MOUSE_DATA.positionY};
}

glm::vec2 Hush::InputManager::GetMouseAcceleration()
{
	return glm::vec2{S_MOUSE_DATA.accelerationX, S_MOUSE_DATA.accelerationY};
}

const glm::vec2 &Hush::InputManager::GetMouseScrollAcceleration()
{
	return S_MOUSE_DATA.wheelAcceleration;
}

void Hush::InputManager::SendKeyEvent(KeyCode key, EKeyState state)
{
	auto mappedKeyCode = static_cast<EKeyCode>(key);
	KeyData data{mappedKeyCode, (EKeyState_t)state};
	// If the key is already inserted and the state is not none
	UpdateKeyStateFromData(data, state);
	S_KEY_DATA_BY_CODE[(uint32_t)mappedKeyCode] = data;
}

void Hush::InputManager::SendGamepadButtonEvent(uint32_t internalGamepadId, EGamepadButton button, EKeyState state)
{
	HUSH_ASSERT((int32_t)button < (int32_t)EGamepadButton::Max && (int32_t)button > (int32_t)EGamepadButton::Invalid,
				"Button {} out of range of valid gamepad buttons", (int32_t)button);
	// Get the controller
	int64_t foundGamepadIdx = -1;
	for (int64_t i = 0; i < (int64_t)S_GAMEPAD_STATES.size(); i++)
	{
		const GamepadState &gamepad = S_GAMEPAD_STATES[i];
		if (gamepad.internalId == internalGamepadId)
		{
			foundGamepadIdx = i;
			break;
		}
	}
	HUSH_COND_FAIL_MSG(foundGamepadIdx != -1,
					   "Unable to find gamepad with internal ID {}, perhaps it was disconnected!", internalGamepadId);
	GamepadState &gamepad = S_GAMEPAD_STATES[foundGamepadIdx];
	GamepadButtonState &buttonState = gamepad.buttons[static_cast<int8_t>(button)];
	GamepadButtonState newStates{(EGamepadButton_t)button, (EKeyState_t)state};
	if (buttonState.currentState != (EKeyState_t)EKeyState::None)
	{
		newStates.previousState = buttonState.currentState;
	}
	if (state == EKeyState::Pressed && (newStates.previousState == (EKeyState_t)EKeyState::Pressed ||
										newStates.previousState == (EKeyState_t)EKeyState::Held))
	{
		newStates.currentState = (EKeyState_t)EKeyState::Held;
	}
	// Finally copy it to the ref
	buttonState = newStates;
}

void Hush::InputManager::SendMouseButtonEvent(MouseButton mouseButton, EKeyState state)
{
	auto mappedButton = static_cast<EMouseButton>(mouseButton);
	S_MOUSE_DATA.mouseButtonMap.insert_or_assign(mappedButton, state);
}

void Hush::InputManager::SendMouseMovementEvent(int32_t posX, int32_t posY, int32_t accelerationX,
												int32_t accelerationY)
{
	S_MOUSE_DATA.positionX = posX;
	S_MOUSE_DATA.positionY = posY;
	S_MOUSE_DATA.accelerationX = accelerationX;
	S_MOUSE_DATA.accelerationY = accelerationY;
}

void Hush::InputManager::SendWheelEvent(float posX, float posY)
{
	S_MOUSE_DATA.wheelAcceleration.x = posX;
	S_MOUSE_DATA.wheelAcceleration.y = posY;
}

void Hush::InputManager::ResetMouseAcceleration()
{
	S_MOUSE_DATA.accelerationX = 0;
	S_MOUSE_DATA.accelerationY = 0;
	S_MOUSE_DATA.wheelAcceleration.x = 0.f;
	S_MOUSE_DATA.wheelAcceleration.y = 0.f;
}

void Hush::InputManager::SendGamepadAxisMotion(uint32_t internalGamepadId, EGamepadAxis axis, float value)
{

	HUSH_ASSERT((int32_t)axis < (int32_t)EGamepadAxis::Max && (int32_t)axis > (int32_t)EGamepadAxis::Invalid,
				"Axis value of {} is outside the range of gamepad axis (0 to {})", (int32_t)axis,
				(int32_t)EGamepadAxis::Max);

	// The compiler will probably optimize this
	for (GamepadState &gamepad : S_GAMEPAD_STATES)
	{
		if (internalGamepadId == gamepad.internalId)
		{
			gamepad.axis[(size_t)axis] = value;
			return;
		}
	}
}

void Hush::InputManager::SetCursorLock(ECursorLockMode lockMode)
{
	HUSH_UNUSED(lockMode);
	// SDL_SetRelativeMouseMode(static_cast<SDL_bool>(lockMode));
}

void Hush::InputManager::AddGamepad(GamepadState state)
{
	// Maybe this needs a check for existing first?
	state.sdlHandle = SDL_OpenGamepad(state.internalId);

	HUSH_COND_FAIL_MSG(state.sdlHandle != nullptr,
					   "Could not open controller with internal ID {}! Unplug and plug it back in to try again",
					   state.internalId);

	// Find an open spot
	for (size_t i = 0; i < MAX_GAMEPADS; i++)
	{
		GamepadState &gamepad = S_GAMEPAD_STATES[i];
		if (gamepad.internalId == 0)
		{
			gamepad = state;
			return;
		}
	}
	SDL_CloseGamepad(reinterpret_cast<SDL_Gamepad *>(state.sdlHandle));
	LogFormat(ELogLevel::Error, "Maximum amount of controllers ({}) exceeded, ignored new controller!", MAX_GAMEPADS);
}

void Hush::InputManager::RemoveGamepad(uint32_t internalGamepadId)
{
	// Find and remove
	for (GamepadState &gamepad : S_GAMEPAD_STATES)
	{
		if (internalGamepadId == gamepad.internalId)
		{
			// zero-init to mark as available
			SDL_CloseGamepad(reinterpret_cast<SDL_Gamepad *>(gamepad.sdlHandle));
			gamepad = {};
			return;
		}
	}
	LogFormat(ELogLevel::Error, "Cannot remove gamepad with internal id {} as it can't be found!", internalGamepadId);
}

float Hush::InputManager::GetGamepadAxis(size_t controllerIndex, EGamepadAxis axis)
{
	HUSH_COND_FAIL_MSG_V(controllerIndex < S_GAMEPAD_STATES.size(), NAN,
						 "Controller index {} out of bounds for available controllers ({})", controllerIndex,
						 S_GAMEPAD_STATES.size());
	HUSH_COND_FAIL_MSG_V((int32_t)axis < (int32_t)EGamepadAxis::Max && (int32_t)axis > (int32_t)EGamepadAxis::Invalid,
						 NAN, "Axis value of {} is outside the range of gamepad axis (0 to {})", (int32_t)axis,
						 (int32_t)EGamepadAxis::Max);
	const GamepadState &gamepad = S_GAMEPAD_STATES[controllerIndex];
	return gamepad.axis[static_cast<size_t>(axis)];
}

bool Hush::InputManager::IsGamepadButtonDown(size_t controllerIndex, EGamepadButton button)
{
	HUSH_COND_FAIL_MSG_V(controllerIndex < S_GAMEPAD_STATES.size(), false,
						 "Controller index {} out of bounds for available controllers ({})", controllerIndex,
						 S_GAMEPAD_STATES.size());
	HUSH_COND_FAIL_MSG_V((int32_t)button < (int32_t)EGamepadButton::Max &&
							 (int32_t)button > (int32_t)EGamepadButton::Invalid,
						 false, "Button value of {} is outside the range of gamepad axis (0 to {})", (int32_t)button,
						 (int32_t)EGamepadButton::Max);
	const GamepadState &gamepad = S_GAMEPAD_STATES[controllerIndex];
	return IS_CURRENTLY_PRESSED(gamepad.buttons[(size_t)button].currentState);
}

bool Hush::InputManager::IsGamepadButtonHeld(size_t controllerIndex, EGamepadButton button)
{
	HUSH_COND_FAIL_MSG_V(controllerIndex < S_GAMEPAD_STATES.size(), false,
						 "Controller index {} out of bounds for available controllers ({})", controllerIndex,
						 S_GAMEPAD_STATES.size());
	HUSH_COND_FAIL_MSG_V((int32_t)button < (int32_t)EGamepadButton::Max &&
							 (int32_t)button > (int32_t)EGamepadButton::Invalid,
						 false, "Button value of {} is outside the range of gamepad axis (0 to {})", (int32_t)button,
						 (int32_t)EGamepadButton::Max);
	const GamepadState &gamepad = S_GAMEPAD_STATES[controllerIndex];
	return gamepad.buttons[(size_t)button].currentState == (EKeyState_t)EKeyState::Held;
}

bool Hush::InputManager::IsGamepadButtonDownThisFrame(size_t controllerIndex, EGamepadButton button)
{
	HUSH_COND_FAIL_MSG_V(controllerIndex < S_GAMEPAD_STATES.size(), false,
						 "Controller index {} out of bounds for available controllers ({})", controllerIndex,
						 S_GAMEPAD_STATES.size());
	HUSH_COND_FAIL_MSG_V((int32_t)button < (int32_t)EGamepadButton::Max &&
							 (int32_t)button > (int32_t)EGamepadButton::Invalid,
						 false, "Button value of {} is outside the range of gamepad axis (0 to {})", (int32_t)button,
						 (int32_t)EGamepadButton::Max);

	const GamepadState &gamepad = S_GAMEPAD_STATES[controllerIndex];
	return gamepad.buttons[(size_t)button].currentState == (EKeyState_t)EKeyState::Pressed;
}

bool Hush::InputManager::IsGamepadButtonUp(size_t controllerIndex, EGamepadButton button)
{

	HUSH_COND_FAIL_MSG_V(controllerIndex < S_GAMEPAD_STATES.size(), false,
						 "Controller index {} out of bounds for available controllers ({})", controllerIndex,
						 S_GAMEPAD_STATES.size());

	HUSH_COND_FAIL_MSG_V((int32_t)button < (int32_t)EGamepadButton::Max &&
							 (int32_t)button > (int32_t)EGamepadButton::Invalid,
						 false, "Button value of {} is outside the range of gamepad axis (0 to {})", (int32_t)button,
						 (int32_t)EGamepadButton::Max);

	const GamepadState &gamepad = S_GAMEPAD_STATES[controllerIndex];
	return gamepad.buttons[(size_t)button].currentState == (EKeyState_t)EKeyState::Released;
}

void Hush::InputManager::UpdateKeyStateFromData(KeyData &keyData, EKeyState incomingState)
{
	// Check if we already had a current state in our entry, and if so, move that to the previous state
	KeyData existingData = S_KEY_DATA_BY_CODE[(uint32_t)keyData.code];
	if (existingData.currentState != (EKeyState_t)EKeyState::None)
	{
		keyData.previousState = existingData.currentState;
	}
	// Check for whether or not the key was pressed before and we should mark it as held
	if (incomingState == EKeyState::Pressed && (keyData.previousState == (EKeyState_t)EKeyState::Pressed ||
												keyData.previousState == (EKeyState_t)EKeyState::Held))
	{
		keyData.currentState = (EKeyState_t)EKeyState::Held;
	}
}

bool Hush::InputManager::MouseMapContains(EMouseButton button)
{
	return S_MOUSE_DATA.mouseButtonMap.find(button) != S_MOUSE_DATA.mouseButtonMap.end();
}
