#pragma once

#include "Vector3Math.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>

namespace Hush
{
	struct Transform
	{
	public:
		Transform() = default;

		Transform(const glm::vec3 &position, const glm::vec3 &scale = Vector3Math::ONE, const glm::quat &rotation = {});

		void SetPosition(const glm::vec3 &position) noexcept;

		[[nodiscard]]
		const glm::vec3 &GetPosition() const noexcept;

		glm::vec3 &GetPosition() noexcept;

		void SetScale(const glm::vec3 &scale) noexcept;

		[[nodiscard]]
		const glm::vec3 &GetScale() const noexcept;

		glm::vec3 &GetScale() noexcept;

		void SetRotationQuat(const glm::quat &rotationQuat) noexcept;

		[[nodiscard]]
		glm::quat GetRotationQuat() const noexcept;

		void SetEulerAngles(const glm::vec3 &euler) noexcept;

		[[nodiscard]]
		glm::vec3 GetEulerAngles() const noexcept;

		[[nodiscard]]
		glm::mat3 GetRotationMatrix() const noexcept;

		[[nodiscard]]
		glm::vec3 Forward() const noexcept;

		[[nodiscard]]
		glm::vec3 Up() const noexcept;

		[[nodiscard]]
		glm::vec3 Right() const noexcept;

		void SetTransformationMatrix(const glm::mat4 &xform);

		[[nodiscard]]
		glm::mat4 GetTransformationMatrix() const;

		[[nodiscard]]
		glm::mat4 XForm(const Transform &other) const;

		glm::mat4 operator*(const Transform &other);

	private:
		glm::vec3 m_position = Vector3Math::ZERO;
		glm::vec3 m_scale = Vector3Math::ONE;
		glm::quat m_rotation{};
	};
} // namespace Hush
