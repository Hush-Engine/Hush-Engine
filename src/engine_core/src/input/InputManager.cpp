#include "InputManager.hpp"
#include "log/Logger.hpp"

#include <magic_enum.hpp>

// TODO: Populate the map in the stack with all enums
// NOLINTNEXTLINE
std::map<KeyCode, KeyData> InputManager::S_KEY_DATA_BY_CODE = {};

bool InputManager::IsKeyDown(KeyCode key)   
{
    // Just to avoid the warning
    (void)key;
    return false;
}

void InputManager::SendKeyEvent(KeyCode key, EKeyState state)
{
    KeyData data{(EKeyCode)key, state};
    bool containsKey = S_KEY_DATA_BY_CODE.find(key) != S_KEY_DATA_BY_CODE.end();
    // If the key is already inserted and the state is not none
    if (containsKey && S_KEY_DATA_BY_CODE[key].currentState != EKeyState::None)
    {
        data.previousState = S_KEY_DATA_BY_CODE[key].currentState;
    }
    std::string_view stateName = magic_enum::enum_name<EKeyCode>(data.code);
    std::string_view currKeyStateName = magic_enum::enum_name<EKeyState>(data.currentState);
    std::string_view prevKeyStateName = magic_enum::enum_name<EKeyState>(data.previousState);
    Hush::LogFormat(Hush::ELogLevel::Info, "Pressed key {} with state: {}, previous state: {}", stateName,
                    currKeyStateName, prevKeyStateName);
    S_KEY_DATA_BY_CODE.insert_or_assign(key, data);
}
