#include "InputManager.hpp"
#include <Logger.hpp>

std::map<KeyCode, KeyData> InputManager::s_keyDataByCode = {
};

bool InputManager::IsKeyDown(KeyCode key)
{
    return false;
}

void InputManager::SendKeyEvent(KeyCode key, EKeyState state)
{
    LOG_INFO_LN("Pressed key %d with state %d", key, state);
    KeyData data {
        key,
        state
    };
    s_keyDataByCode.insert_or_assign(key, data);
}
