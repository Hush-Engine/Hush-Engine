/*! \file InputManager.hpp
    \author Kyn21kx
    \date 2024-02-28
    \brief Receives the input information of the application and makes it available through static methods
*/

#pragma once
#include "definitions/KeyData.hpp"
#include "definitions/MouseButton.hpp"
#include "definitions/MouseData.hpp"
#include <glm/vec2.hpp>
#include <map>

class InputManager
{
  public:
    /// <summary>
    /// Evaluates to true the frame the key is identified as <seealso cref="EKeyState::Pressed"/>
    /// </summary>
    static bool IsKeyDown(EKeyCode key);

    /// <summary>
    /// Evaluates to true the frame the key is identified as <seealso cref="EKeyState::Release"/>
    /// </summary>
    static bool IsKeyUp(EKeyCode key);

    /// <summary>
    /// Evaluates to true as long as the key is identified as <seealso cref="EKeyState::Held"/>
    /// </summary>
    static bool IsKeyHeld(EKeyCode key);

    /// <summary>
    /// Evaluates to true for as long as the mouse button is pressed
    /// </summary>
    static bool GetMouseButtonPressed(EMouseButton button);

    /// <summary>
    /// Gets the vector of the mouse's position in pixels
    /// </summary>
    static glm::vec2 GetMousePosition();

    /// <summary>
    /// Gets the vector of the mouse's acceleration in pixels/s^2
    /// </summary>
    static glm::vec2 GetMouseAcceleration();

#pragma region Methods to send events from SDL

    static void SendKeyEvent(KeyCode key, EKeyState state);

    static void SendMouseButtonEvent(MouseButton mouseButton, EKeyState state);

    static void SendMouseMovementEvent(int32_t posX, int32_t posY, int32_t accelerationX, int32_t accelerationY);

#pragma endregion
  private:
    // TODO: Reserve memory for this map???
    // NOLINTNEXTLINE
    static std::map<EKeyCode, KeyData> S_KEY_DATA_BY_CODE;

    // NOLINTNEXTLINE
    static MouseData S_MOUSE_DATA;

    static void UpdateKeyStateFromData(KeyData &keyData, EKeyState incomingState);

    static bool KeyMapContains(EKeyCode key);

    static bool MouseMapContains(EMouseButton button);
};
