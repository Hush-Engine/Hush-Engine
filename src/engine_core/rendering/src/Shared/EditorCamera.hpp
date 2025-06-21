#pragma once
#include "Camera.hpp"

namespace Hush
{
	class EditorCamera final : public Camera
	{
	public:
		EditorCamera() = default;

		EditorCamera(float degFov, float width, float height, float nearP, float farP);

		void OnUpdate(float delta);

		[[nodiscard]]
		glm::mat4 GetViewMatrix() const noexcept;

		[[nodiscard]]
		glm::mat4 GetOrientationMatrix() const noexcept;

		[[nodiscard]]
		glm::vec3 GetPosition() const noexcept;

	private:
		float ApplyAccelerationCurve(float blend);

		glm::vec3 m_position{};
		float m_yaw{}, m_pitch{};
		float m_blendValue = 0.0F;
	};
} // namespace Hush
