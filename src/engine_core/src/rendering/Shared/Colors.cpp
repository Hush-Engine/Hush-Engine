#include "Colors.hpp"

uint32_t Hush::Color::ColorAsInteger(glm::vec4 const &color)
{
    return Hush::Color::PackUnorm4x8(color);
}
