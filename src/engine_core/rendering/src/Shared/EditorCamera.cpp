#include "EditorCamera.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <glm/gtx/quaternion.hpp>

Hush::EditorCamera::EditorCamera(float degFov, float width, float height, float nearP, float farP)
	: Camera(degFov, width, height, nearP, farP), m_yaw(0.0F), m_pitch(0.0F)
{
	this->m_position = glm::vec3(0.F, 1.F, 5.F);
}

glm::mat4 Hush::EditorCamera::GetOrientationMatrix() const noexcept
{
	glm::quat pitchRotation = glm::angleAxis(this->m_pitch, glm::vec3{-1.f, 0.f, 0.f});
	glm::quat yawRotation = glm::angleAxis(this->m_yaw, glm::vec3{0.f, -1.f, 0.f});

	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

const glm::vec3& Hush::EditorCamera::GetPosition() const noexcept
{
	return this->m_position;
}

glm::vec3& Hush::EditorCamera::GetPosition() noexcept
{
	return this->m_position;
}

glm::mat4 Hush::EditorCamera::GetViewMatrix() const noexcept
{
	glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), this->m_position);
	glm::mat4 cameraRotation = this->GetOrientationMatrix();
	glm::mat4 viewMatrix = cameraTranslation * cameraRotation;
	return glm::inverse(viewMatrix);
}

float& Hush::EditorCamera::GetYaw() noexcept {
	return this->m_yaw;
}

float& Hush::EditorCamera::GetPitch() noexcept {
	return this->m_pitch;
}
