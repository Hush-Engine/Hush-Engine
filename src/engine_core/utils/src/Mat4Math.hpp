#pragma once

#include "Vector3Math.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Hush::Mat4Math {
	constexpr glm::mat4 IDENTITY = glm::mat4(1.0F);

	constexpr size_t TRANSLATION_COLUMN = 3;
	
	inline void DecomposeTRS(const glm::mat4& matrix, glm::vec3& translation, glm::quat& rotation, glm::vec3& scale) {
		translation = glm::vec3(matrix[TRANSLATION_COLUMN]);
		glm::vec3 basisX(matrix[0]);
		glm::vec3 basisY(matrix[1]);
		glm::vec3 basisZ(matrix[2]);
		scale.x = Vector3Math::Magnitude(basisX);
		scale.y = Vector3Math::Magnitude(basisY);
		scale.z = Vector3Math::Magnitude(basisZ);
		glm::mat3 rotationMat(
			basisX / scale.x,
			basisY / scale.y,
			basisZ / scale.z
		);
		rotation = glm::quat_cast(rotationMat);
	}
	
	inline glm::mat4 ComposeTRS(const glm::vec3& position, const glm::quat& rotation, const glm::vec3& scale) {
		glm::mat4 result = glm::translate(Mat4Math::IDENTITY, position);
		result *= glm::mat4_cast(rotation);
		result = glm::scale(result, scale);
		return result;
	}
	
}

