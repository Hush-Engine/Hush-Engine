#include "InputManager.hpp"
#include "log/Logger.hpp"
#include <magic_enum.hpp>

// TODO: Populate the map in the stack with all enums
// NOLINTNEXTLINE
std::map<EKeyCode, KeyData> InputManager::S_KEY_DATA_BY_CODE = {};
// NOLINTNEXTLINE
MouseData InputManager::S_MOUSE_DATA = {};

bool InputManager::IsKeyDown(EKeyCode key)
{
    return KeyMapContains(key) && S_KEY_DATA_BY_CODE[key].currentState == EKeyState::Pressed;
}

bool InputManager::IsKeyUp(EKeyCode key)
{
    return KeyMapContains(key) && S_KEY_DATA_BY_CODE[key].currentState == EKeyState::Released;
}

bool InputManager::IsKeyHeld(EKeyCode key)
{
    return KeyMapContains(key) && S_KEY_DATA_BY_CODE[key].currentState == EKeyState::Held;
}

bool InputManager::GetMouseButtonPressed(EMouseButton button)
{
    return MouseMapContains(button) && S_MOUSE_DATA.mouseButtonMap[button] == EKeyState::Pressed;
}

glm::vec2 InputManager::GetMousePosition()
{
    return glm::vec2{S_MOUSE_DATA.positionX, S_MOUSE_DATA.positionY};
}

glm::vec2 InputManager::GetMouseAcceleration()
{
    return glm::vec2{S_MOUSE_DATA.accelerationX, S_MOUSE_DATA.accelerationY};
}

void InputManager::SendKeyEvent(KeyCode key, EKeyState state)
{
    auto mappedKeyCode = static_cast<EKeyCode>(key);
    KeyData data{mappedKeyCode, state};
    // If the key is already inserted and the state is not none
    if (KeyMapContains(mappedKeyCode))
    {
        UpdateKeyStateFromData(data, state);
    }
    S_KEY_DATA_BY_CODE.insert_or_assign(mappedKeyCode, data);
}

void InputManager::SendMouseButtonEvent(MouseButton mouseButton, EKeyState state)
{
    auto mappedButton = static_cast<EMouseButton>(mouseButton);
    std::string_view stateName = magic_enum::enum_name<EKeyState>(state);
    std::string_view btnName = magic_enum::enum_name<EMouseButton>(mappedButton);
    S_MOUSE_DATA.mouseButtonMap.insert_or_assign(mappedButton, state);
    Hush::LogFormat(Hush::ELogLevel::Info, "Pressed mouse button {} with state {}", btnName, stateName);
}

void InputManager::SendMouseMovementEvent(int32_t posX, int32_t posY, int32_t accelerationX, int32_t accelerationY)
{
    S_MOUSE_DATA.positionX = posX;
    S_MOUSE_DATA.positionY = posY;
    S_MOUSE_DATA.accelerationX = accelerationX;
    S_MOUSE_DATA.accelerationY = accelerationY;
    Hush::LogFormat(Hush::ELogLevel::Info, "Mouse pos: ({}, {})\nMouse Acceleration: ({}, {})", S_MOUSE_DATA.positionX,
                    S_MOUSE_DATA.positionY, S_MOUSE_DATA.accelerationX, S_MOUSE_DATA.accelerationY);
}

void InputManager::ResetMouseAcceleration()
{
    S_MOUSE_DATA.accelerationX = 0;
    S_MOUSE_DATA.accelerationY = 0;
}

void InputManager::UpdateKeyStateFromData(KeyData &keyData, EKeyState incomingState)
{
    // Check if we already had a current state in our entry, and if so, move that to the previous state
    KeyData existingData = S_KEY_DATA_BY_CODE[keyData.code];
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

bool InputManager::KeyMapContains(EKeyCode key)
{
    return S_KEY_DATA_BY_CODE.find(key) != S_KEY_DATA_BY_CODE.end();
}

bool InputManager::MouseMapContains(EMouseButton button)
{
    return S_MOUSE_DATA.mouseButtonMap.find(button) != S_MOUSE_DATA.mouseButtonMap.end();
}
