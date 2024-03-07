/*! \file MouseData.hpp
    \author Kyn21kx
    \date 2024-03-06
    \brief Structure that holds the current data of the mouse, as interpreted by SDL events
*/

#pragma once
#include "KeyStates.hpp"
#include "MouseButton.hpp"
#include <unordered_map>

struct MouseData
{
    std::unordered_map<EMouseButton, EKeyState> mouseButtonMap{};
    int32_t positionX = 0;
    int32_t positionY = 0;
    int32_t accelerationX = 0;
    int32_t accelerationY = 0;
};
