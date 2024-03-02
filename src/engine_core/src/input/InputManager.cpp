#include "InputManager.hpp"

// TODO: Populate the map in the stack with all enums
std::map<KeyCode, KeyData> InputManager::s_keyDataByCode = {};

bool InputManager::IsKeyDown(KeyCode key)
{
    return false;
}

void InputManager::SendKeyEvent(KeyCode key, EKeyState state)
{
    KeyData data{(EKeyCode)key, state};
    bool containsKey = s_keyDataByCode.find(key) != s_keyDataByCode.end();
    // If the key is already inserted and the state is not none
    if (containsKey && s_keyDataByCode[key].currentState != EKeyState::None)
    {
        data.previousState = s_keyDataByCode[key].currentState;
    }
    std::string_view stateName = magic_enum::enum_name<EKeyCode>(data.code);
    std::string_view currKeyStateName = magic_enum::enum_name<EKeyState>(data.currentState);
    std::string_view prevKeyStateName = magic_enum::enum_name<EKeyState>(data.previousState);
    LOG_INFO_LN("Pressed key %s with state: %s, previous state: %s", stateName.data(), currKeyStateName.data(),
                prevKeyStateName.data());
    s_keyDataByCode.insert_or_assign(key, data);
}
