/*! \file KeyData.hpp
    \author Kyn21kx
    \date 2024-02-28
    \brief Stores information about a Key with its related current and previous state
*/

#pragma once
#include "KeyCode.hpp"
#include "KeyStates.hpp"

struct KeyData
{
    EKeyCode code;
    EKeyState currentState = EKeyState::None;
    EKeyState previousState = EKeyState::None;
};