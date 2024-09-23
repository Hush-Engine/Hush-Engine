/*! \file KeyStates.hpp
    \author Kyn21kx
    \date 2024-02-28
    \brief Represents the possible states of a key on an input event
*/

#pragma once

enum class EKeyState
{
    None = -1,
    Pressed,
    Held,
    Released
};