#include "InputManager.hpp"
#include "log/Logger.hpp"
#include <magic_enum.hpp>
#include "editor/DebugUI.hpp"

// TODO: Populate the map in the stack with all enums
// NOLINTNEXTLINE
std::unordered_map<EKeyCode, KeyData> Hush::InputManager::S_KEY_DATA_BY_CODE = {};
// NOLINTNEXTLINE
MouseData Hush::InputManager::S_MOUSE_DATA = {};

bool Hush::InputManager::IsKeyDown(EKeyCode key)
{
    return KeyMapContains(key) && S_KEY_DATA_BY_CODE[key].currentState == EKeyState::Pressed;
}

bool Hush::InputManager::IsKeyUp(EKeyCode key)
{
    return KeyMapContains(key) && S_KEY_DATA_BY_CODE[key].currentState == EKeyState::Released;
}

bool Hush::InputManager::IsKeyHeld(EKeyCode key)
{
    return KeyMapContains(key) && S_KEY_DATA_BY_CODE[key].currentState == EKeyState::Held;
}

bool Hush::InputManager::GetMouseButtonPressed(EMouseButton button)
{
    return MouseMapContains(button) && S_MOUSE_DATA.mouseButtonMap[button] == EKeyState::Pressed;
}

glm::vec2 Hush::InputManager::GetMousePosition()
{
    return glm::vec2{S_MOUSE_DATA.positionX, S_MOUSE_DATA.positionY};
}

glm::vec2 Hush::InputManager::GetMouseAcceleration()
{
    return glm::vec2{S_MOUSE_DATA.accelerationX, S_MOUSE_DATA.accelerationY};
}

void Hush::InputManager::SendKeyEvent(KeyCode key, EKeyState state)
{
    auto mappedKeyCode = static_cast<EKeyCode>(key);
    KeyData data{mappedKeyCode, state};
    // If the key is already inserted and the state is not none
    if (KeyMapContains(mappedKeyCode))
    {
        UpdateKeyStateFromData(data, state);
    }
    if (DebugUI::KeyboardInfoEnabled())
    {
        LogFormat(ELogLevel::Info, "Key {} pressed with state {}!", magic_enum::enum_name(mappedKeyCode),
                  magic_enum::enum_name(data.currentState));
    }
    S_KEY_DATA_BY_CODE.insert_or_assign(mappedKeyCode, data);
}

void Hush::InputManager::SendMouseButtonEvent(MouseButton mouseButton, EKeyState state)
{
    auto mappedButton = static_cast<EMouseButton>(mouseButton);
    S_MOUSE_DATA.mouseButtonMap.insert_or_assign(mappedButton, state);
    if (DebugUI::MouseInfoEnabled()) {
        LogFormat(ELogLevel::Info, "Mouse button {} pressed with state {}!", magic_enum::enum_name(mappedButton),
                  magic_enum::enum_name(state));
    }
}

void Hush::InputManager::SendMouseMovementEvent(int32_t posX, int32_t posY, int32_t accelerationX,
                                                int32_t accelerationY)
{
    S_MOUSE_DATA.positionX = posX;
    S_MOUSE_DATA.positionY = posY;
    S_MOUSE_DATA.accelerationX = accelerationX;
    S_MOUSE_DATA.accelerationY = accelerationY;
    if (DebugUI::MouseInfoEnabled())
    {
        LogFormat(ELogLevel::Info, "Mouse position: ({}, {})", S_MOUSE_DATA.positionX, S_MOUSE_DATA.positionY);
        LogFormat(ELogLevel::Info, "Mouse acceleration: ({}, {})", S_MOUSE_DATA.accelerationX, S_MOUSE_DATA.accelerationY);
    }
}

void Hush::InputManager::ResetMouseAcceleration()
{
    S_MOUSE_DATA.accelerationX = 0;
    S_MOUSE_DATA.accelerationY = 0;
}

void Hush::InputManager::UpdateKeyStateFromData(KeyData &keyData, EKeyState incomingState)
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

bool Hush::InputManager::KeyMapContains(EKeyCode key)
{
    return S_KEY_DATA_BY_CODE.find(key) != S_KEY_DATA_BY_CODE.end();
}

bool Hush::InputManager::MouseMapContains(EMouseButton button)
{
    return S_MOUSE_DATA.mouseButtonMap.find(button) != S_MOUSE_DATA.mouseButtonMap.end();
}
