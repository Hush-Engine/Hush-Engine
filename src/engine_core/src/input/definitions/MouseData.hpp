/*! \file MouseData.hpp
    \author Kyn21kx
    \date 2024-03-06
    \brief Structure that holds the current data of the mouse, as interpreted by SDL events
*/

#pragma once
#include "MouseButton.hpp"
#include "KeyStates.hpp"
#include <map>

struct MouseData
{
    std::map<EMouseButton, EKeyState> mouseButtonMap{};
    int positionX;
    int positionY;

};
