/*! \file InputManager.hpp
    \author Kyn21kx
    \date 2024-02-28
    \brief Receives the input information of the application and makes it available through static methods
*/

#pragma once
#include "KeyData.hpp"
#include <map>

class InputManager
{
  public:
    static bool IsKeyDown(KeyCode key);

    static void SendKeyEvent(KeyCode key, EKeyState state);

  private:
    // TODO: Reserve memory for this map???
    // NOLINTNEXTLINE
    static std::map<KeyCode, KeyData> S_KEY_DATA_BY_CODE;
};
