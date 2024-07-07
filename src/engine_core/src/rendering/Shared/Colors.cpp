#include "Colors.hpp"

constexpr uint32_t Hush::Color::ColorAsInteger(glm::vec4 const &color)
{
    return Hush::Color::CompileTimePackUnorm4x8(color);
}
