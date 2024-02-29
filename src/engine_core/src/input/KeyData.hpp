/*! \file KeyData.hpp
    \author Kyn21kx
    \date 2024-02-28
    \brief Stores information about a Key with its related current and previous state
*/

#pragma once
#include "KeyStates.hpp"
using KeyCode = int;

struct KeyData
{
    KeyCode code;
    EKeyState currentState;
    EKeyState previousState;
};