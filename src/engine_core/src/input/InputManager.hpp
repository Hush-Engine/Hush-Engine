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
#include <unordered_map>
namespace Hush
{
    class InputManager
    {
      public:
        /// @brief Evaluates to true the frame the key is identified as EKeyState::Pressed
        static bool IsKeyDown(EKeyCode key);

        /// @brief Evaluates to true the frame the key is identified as EKeyState::Release
        static bool IsKeyUp(EKeyCode key);

        /// @brief Evaluates to true as long as the key is identified asEKeyState::Held
        static bool IsKeyHeld(EKeyCode key);

        /// @brief Evaluates to true for as long as the mouse button is pressed
        static bool GetMouseButtonPressed(EMouseButton button);

        /// @brief Gets the vector of the mouse's position in pixels
        static glm::vec2 GetMousePosition();

        /// @brief Gets the vector of the mouse's acceleration in pixels/s^2
        static glm::vec2 GetMouseAcceleration();

        /* Methods to send events from SDL */

        static void SendKeyEvent(KeyCode key, EKeyState state);

        static void SendMouseButtonEvent(MouseButton mouseButton, EKeyState state);

        static void SendMouseMovementEvent(int32_t posX, int32_t posY, int32_t accelerationX, int32_t accelerationY);

        static void ResetMouseAcceleration();

      private:
        // TODO: Reserve memory for this map???
        // NOLINTNEXTLINE
        static std::unordered_map<EKeyCode, KeyData> S_KEY_DATA_BY_CODE;

        // NOLINTNEXTLINE
        static MouseData S_MOUSE_DATA;

        static void UpdateKeyStateFromData(KeyData &keyData, EKeyState incomingState);

        static bool KeyMapContains(EKeyCode key);

        static bool MouseMapContains(EMouseButton button);
    };
} // namespace Hush
