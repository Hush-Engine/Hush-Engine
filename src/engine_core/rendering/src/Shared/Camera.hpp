/*! \file Camera.hpp
	\author Kyn21kx
	\date 2024-05-30
	\brief Camera descriptor class for both scene and editor rendering
*/

#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>

namespace Hush
{
	class Camera
	{
	public:
		Camera() = default;
		Camera(const Camera &) = default;
		Camera(Camera &&) = delete;
		Camera &operator=(const Camera &) = default;
		Camera &operator=(Camera &&) = delete;
		Camera(const glm::mat4 &projectionMat, const glm::mat4 &unreversedProjectionMat) noexcept;
		Camera(float degFov, float width, float height, float nearP, float farP) noexcept;
		virtual ~Camera() = default;

		[[nodiscard]]
		inline glm::mat4 GetProjectionMatrix() const noexcept
		{
			glm::mat4 proj = glm::perspective(glm::radians(this->m_fov), this->m_viewportSize.x / this->m_viewportSize.y,
									this->m_farPlane, this->m_nearPlane);
			proj[1][1] *= -1;
			return proj;
		}

		[[nodiscard]]
		const glm::mat4 &GetUnreversedProjectionMatrix() const noexcept;

		void SetProjectionMatrix(glm::mat4 projection, glm::mat4 unReversedProjection);

		void SetPerspectiveProjectionMatrix(const float radFov, const float width, const float height,
											const float nearP, const float farP);

		[[nodiscard]]
		float GetFarPlane() const noexcept;

		/// @brief Update the viewport dimensions (e.g. when the scene panel resizes).
		/// This affects the aspect ratio used by GetProjectionMatrix().
		void SetViewportSize(float width, float height) noexcept
		{
			m_viewportSize = {width, height};
		}

		/// @brief Returns the current viewport size.
		[[nodiscard]]
		glm::vec2 GetViewportSize() const noexcept
		{
			return m_viewportSize;
		}

	protected:
		// NOLINTNEXTLINE
		float m_exposure = 0.8f; // Aribtrary value (inspired from the Hazel Engine)
	private:
		float m_fov{};
		glm::vec2 m_viewportSize{};
		float m_nearPlane{};
		float m_farPlane{};
		glm::mat4 m_projectionMatrix = glm::mat4(1.0f);
		// Currently only needed for shadow maps and ImGuizmo
		glm::mat4 m_unreversedProjectionMatrix = glm::mat4(1.0f);
	};
} // namespace Hush
