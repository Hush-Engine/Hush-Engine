#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

namespace Hush::Vector4Math
{

	constexpr inline glm::vec4 ONE = glm::vec4(1.0F);

	constexpr inline glm::vec4 FromVec3(const glm::vec3 &vec, float w)
	{
		return {vec.x, vec.y, vec.z, w};
	}

} // namespace Hush::Vector4Math
