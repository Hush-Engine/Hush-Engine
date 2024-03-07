/*! \file MouseButton.hpp
    \author Kyn21kx
    \date 2024-03-06
    \brief Enum to handle possible mouse buttons coming from SDL2
*/

#pragma once
#include <cstdint>

/// <summary>
/// Alias for the SDL type coming into Hush
/// </summary>
using MouseButton = uint8_t;

enum class EMouseButton : MouseButton
{
    Left = 1U,
    Middle = 2U,
    Right = 3U,
    X1 = 4U,
    X2 = 5U
};
