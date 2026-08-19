#pragma once
#include "Camera.hpp"

namespace Hush
{

	#include <Hushgen.hpp>
	#include <reflection/Type.hpp>
	#include <serialization/Serialization.hpp>
	#include <serialization/Deserialization.hpp>

	#if __has_include("EditorCamera.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
	#include "EditorCamera.hushgen.hpp"
	#endif

	class [[hush::reflect]] EditorCamera final : public Camera
	{
		HUSH_GENERATED_BODY
	public:
		EditorCamera() = default;

		EditorCamera(const EditorCamera &) = default;
		EditorCamera(EditorCamera &&) = delete;
		EditorCamera &operator=(const EditorCamera &) = default;
		EditorCamera &operator=(EditorCamera &&) = delete;
		EditorCamera(float degFov, float width, float height, float nearP, float farP);

		~EditorCamera() override = default;

		[[nodiscard]]
		glm::mat4 GetViewMatrix() const noexcept;

		[[nodiscard]]
		glm::mat4 GetOrientationMatrix() const noexcept;

		[[nodiscard]]
		const glm::vec3 &GetPosition() const noexcept;

		[[nodiscard]]
		glm::vec3 &GetPosition() noexcept;

		float &GetPitch() noexcept;

		float &GetYaw() noexcept;

	private:
		float ApplyAccelerationCurve(float blend);

		glm::vec3 m_position{};
		float m_yaw{}, m_pitch{};
		float m_blendValue = 0.0F;
	};
} // namespace Hush
