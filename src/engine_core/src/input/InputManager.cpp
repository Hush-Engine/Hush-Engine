#include "InputManager.hpp"
#include "log/Logger.hpp"

#include <magic_enum.hpp>

// TODO: Populate the map in the stack with all enums
// NOLINTNEXTLINE
std::map<KeyCode, KeyData> InputManager::S_KEY_DATA_BY_CODE = {};

bool InputManager::IsKeyDown(KeyCode key)
{
    return KeyMapContains(key) && S_KEY_DATA_BY_CODE[key].currentState == EKeyState::Pressed;
}

bool InputManager::IsKeyUp(KeyCode key)
{
    return KeyMapContains(key) && S_KEY_DATA_BY_CODE[key].currentState == EKeyState::Released;
}

bool InputManager::IsKeyHeld(KeyCode key)
{
    return KeyMapContains(key) && S_KEY_DATA_BY_CODE[key].currentState == EKeyState::Held;
}

void InputManager::SendKeyEvent(KeyCode key, EKeyState state)
{
    KeyData data{
        static_cast<EKeyCode>(key), 
        state
    };
    // If the key is already inserted and the state is not none
    if (KeyMapContains(key))
    {
        UpdateKeyStateFromData(data, state);
    }
    std::string_view stateName = magic_enum::enum_name<EKeyCode>(data.code);
    std::string_view currKeyStateName = magic_enum::enum_name<EKeyState>(data.currentState);
    std::string_view prevKeyStateName = magic_enum::enum_name<EKeyState>(data.previousState);
    Hush::LogFormat(Hush::ELogLevel::Info, "Pressed key {} with state: {}, previous state: {}", stateName,
                    currKeyStateName, prevKeyStateName);
    S_KEY_DATA_BY_CODE.insert_or_assign(key, data);
}

void InputManager::SendMouseButtonEvent(MouseButton mouseButton, EKeyState state)
{
    auto mappedButton = static_cast<EMouseButton>(mouseButton);
    std::string_view stateName = magic_enum::enum_name<EKeyState>(state);
    std::string_view btnName = magic_enum::enum_name<EMouseButton>(mappedButton);
    Hush::LogFormat(Hush::ELogLevel::Info, "Pressed mouse button {} with state {}", btnName, stateName);
}

void InputManager::UpdateKeyStateFromData(KeyData &keyData, EKeyState incomingState)
{
    auto key = static_cast<KeyCode>(keyData.code);
    //Check if we already had a current state in our entry, and if so, move that to the previous state
    KeyData existingData = S_KEY_DATA_BY_CODE[key];
    if (existingData.currentState != EKeyState::None)
    {
        keyData.previousState = existingData.currentState;
    }
    //Check for whether or not the key was pressed before and we should mark it as held
    if (incomingState == EKeyState::Pressed && (keyData.previousState == EKeyState::Pressed ||
                                        keyData.previousState == EKeyState::Held))
    {
        keyData.currentState = EKeyState::Held;
    }
}

bool InputManager::KeyMapContains(KeyCode key)
{
    return S_KEY_DATA_BY_CODE.find(key) != S_KEY_DATA_BY_CODE.end();
}
