/*! \file InputManager.hpp
    \author Kyn21kx
    \date 2024-02-28
    \brief Receives the input information of the application and makes it available through static methods
*/

#pragma once
#include "definitions/KeyData.hpp"
#include "definitions/MouseButton.hpp"
#include <map>

class InputManager
{
  public:
    /// <summary>
    /// Evaluates to true the frame the key is identified as <seealso cref="EKeyState::Pressed"/> 
    /// </summary>
    static bool IsKeyDown(KeyCode key);

    /// <summary>
    /// Evaluates to true the frame the key is identified as <seealso cref="EKeyState::Release"/>
    /// </summary>
    static bool IsKeyUp(KeyCode key);

    /// <summary>
    /// Evaluates to true as long as the key is identified as <seealso cref="EKeyState::Held"/>
    /// </summary>
    static bool IsKeyHeld(KeyCode key);

    static void SendKeyEvent(KeyCode key, EKeyState state);

    static void SendMouseButtonEvent(MouseButton mouseButton, EKeyState state);

  private:
    // TODO: Reserve memory for this map???
    // NOLINTNEXTLINE
    static std::map<KeyCode, KeyData> S_KEY_DATA_BY_CODE;

    static void UpdateKeyStateFromData(KeyData &keyData, EKeyState incomingState);

    static bool KeyMapContains(KeyCode key);
};
