/*! \file RenderStats.hpp
    \author Kyn21kx
    \date 2024-06-12
    \brief Structure to hold information about the current renderer's stats
*/

#pragma once
#include <cstdint>

struct RenderStats
{
    uint32_t drawCalls;
    uint32_t triangles;
    uint32_t frameIndex;
    float fps;
};