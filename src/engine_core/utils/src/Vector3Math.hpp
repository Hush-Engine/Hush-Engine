#pragma once

#include "MathUtils.hpp"
#include <cmath>
#include <glm/ext/vector_float4.hpp>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace Hush::Vector3Math
{

	constexpr inline glm::vec3 ZERO = glm::vec3(0.0F);

	constexpr inline glm::vec3 ONE = glm::vec3(1.0F);

	constexpr inline glm::vec3 RIGHT = glm::vec3(1.0F, 0.0F, 0.0F);

	constexpr inline glm::vec3 UP = glm::vec3(0.0F, 1.0F, 0.0F);

	constexpr inline glm::vec3 FORWARD = glm::vec3(0.0F, 0.0F, -1.0F);

	inline glm::vec3 Lerp(glm::vec3 from, glm::vec3 to, float t)
	{
		return {MathUtils::Lerp(from.x, to.x, t), MathUtils::Lerp(from.y, to.y, t), MathUtils::Lerp(from.z, to.z, t)};
	}

	/// @brief Performs a narrowing conversion from vec4 to vec3
	constexpr glm::vec3 FromVec4(const glm::vec4 &vec)
	{
		return {vec};
	}

	inline float Magnitude(const glm::vec3 &vec)
	{
		return glm::length(vec);
	}

} // namespace Hush::Vector3Math
