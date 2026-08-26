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
#include <HushBindings.hpp>
namespace Hush
{

	enum class [[hush::export]] ECursorLockMode
	{
		Free = 0,
		Locked = 1
	};

	class InputManager
	{
	public:
		/// @brief Evaluates to true whilst the key is pressed down
		[[hush::export]]
		static bool IsKeyDown(EKeyCode key);

		/// @brief Evaluates to true the frame the key is identified as EKeyState::Pressed
		[[hush::export]]
		static bool IsKeyDownThisFrame(EKeyCode key);

		/// @brief Evaluates to true the frame the key is identified as EKeyState::Release
		[[hush::export]]
		static bool IsKeyUp(EKeyCode key);

		/// @brief Evaluates to true as long as the key is identified asEKeyState::Held
		[[hush::export]]
		static bool IsKeyHeld(EKeyCode key);

		/// @brief Evaluates to true for as long as the mouse button is pressed
		[[hush::export]]
		static bool GetMouseButtonPressed(EMouseButton button);

		[[hush::export]]
		static bool FetchCharThisFrame(char *outChar);

		/// @brief Gets the vector of the mouse's position in pixels
		[[hush::export]]
		static glm::vec2 GetMousePosition();

		/// @brief Gets the vector of the mouse's acceleration in pixels/s^2
		static glm::vec2 GetMouseAcceleration();

		static const glm::vec2 &GetMouseScrollAcceleration();

		/* Methods to send events from SDL */

		static void SendKeyEvent(KeyCode key, EKeyState state);

		static void SendMouseButtonEvent(MouseButton mouseButton, EKeyState state);

		static void SendMouseMovementEvent(int32_t posX, int32_t posY, int32_t accelerationX, int32_t accelerationY);

		static void SendWheelEvent(float posX, float posY);

		static void ResetMouseAcceleration();

		static void ResetCharData();

		static void SendCharEvent(char pressedChar);

		[[hush::export]]
		static void SetCursorLock(ECursorLockMode lockMode);

	private:
		// TODO: Reserve memory for this map???
		// NOLINTNEXTLINE
		static std::unordered_map<EKeyCode, KeyData> S_KEY_DATA_BY_CODE;

		// NOLINTNEXTLINE
		static MouseData S_MOUSE_DATA;

		// NOLINTNEXTLINE
		static inline char s_lastChar = 0;

		static void UpdateKeyStateFromData(KeyData &keyData, EKeyState incomingState);

		static bool KeyMapContains(EKeyCode key);

		static bool MouseMapContains(EMouseButton button);
	};
} // namespace Hush
