#include "InputManager.hpp"
#include "Platform.hpp"
#include "definitions/KeyCode.hpp"
#include "definitions/KeyStates.hpp"
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <SDL3/SDL_mouse.h>

#define IS_CURRENTLY_PRESSED(key) (key == EKeyState::Pressed || key == EKeyState::Held)

// TODO: Populate the map in the stack with all enums
// NOLINTNEXTLINE
std::vector<Hush::KeyData> Hush::InputManager::S_KEY_DATA_BY_CODE = {};
// NOLINTNEXTLINE
Hush::MouseData Hush::InputManager::S_MOUSE_DATA = {};

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
	return S_KEY_DATA_BY_CODE[(uint32_t)key].currentState == EKeyState::Pressed;
}

bool Hush::InputManager::IsKeyUp(EKeyCode key)
{
	return S_KEY_DATA_BY_CODE[(uint32_t)key].currentState == EKeyState::Released;
}

bool Hush::InputManager::IsKeyHeld(EKeyCode key)
{
	return S_KEY_DATA_BY_CODE[(uint32_t)key].currentState == EKeyState::Held;
}

bool Hush::InputManager::GetMouseButtonPressed(EMouseButton button)
{
	return MouseMapContains(button) && IS_CURRENTLY_PRESSED(S_MOUSE_DATA.mouseButtonMap[button]);
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
		if (ref.currentState == EKeyState::Pressed)
		{
			// Will be marked as held, and it's SDL's job to mark it as released
			ref.currentState = EKeyState::Held;
			ref.previousState = EKeyState::Pressed;
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
	KeyData data{mappedKeyCode, state};
	// If the key is already inserted and the state is not none
	UpdateKeyStateFromData(data, state);
	S_KEY_DATA_BY_CODE[(uint32_t)mappedKeyCode] = data;
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

void Hush::InputManager::SetCursorLock(ECursorLockMode lockMode)
{
	HUSH_UNUSED(lockMode);
	// SDL_SetRelativeMouseMode(static_cast<SDL_bool>(lockMode));
}

void Hush::InputManager::UpdateKeyStateFromData(KeyData &keyData, EKeyState incomingState)
{
	// Check if we already had a current state in our entry, and if so, move that to the previous state
	KeyData existingData = S_KEY_DATA_BY_CODE[(uint32_t)keyData.code];
	if (existingData.currentState != EKeyState::None)
	{
		keyData.previousState = existingData.currentState;
	}
	// Check for whether or not the key was pressed before and we should mark it as held
	if (incomingState == EKeyState::Pressed &&
		(keyData.previousState == EKeyState::Pressed || keyData.previousState == EKeyState::Held))
	{
		keyData.currentState = EKeyState::Held;
	}
}

bool Hush::InputManager::MouseMapContains(EMouseButton button)
{
	return S_MOUSE_DATA.mouseButtonMap.find(button) != S_MOUSE_DATA.mouseButtonMap.end();
}
