#include "EditorCamera.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "InputManager.hpp"
#include <glm/gtx/quaternion.hpp>
#include "Vector3Math.hpp"

Hush::EditorCamera::EditorCamera(float degFov, float width, float height, float nearP, float farP)
	: Camera(degFov, width, height, nearP, farP)
{
	this->m_position = glm::vec3(0.f, 1.f, 5.f);
	this->m_yaw = 0.0f;
	this->m_pitch = 0.0f;
}

void Hush::EditorCamera::OnUpdate(float delta)
{

	glm::mat4 viewMatrix = this->GetViewMatrix();
	glm::vec3 forward = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
	if (InputManager::GetMouseScrollAcceleration().y != 0.0f)
	{
		constexpr float zoomSpeed = 100.f;
		this->m_position += forward * InputManager::GetMouseScrollAcceleration().y * zoomSpeed * delta;
	}
	
	if (!InputManager::GetMouseButtonPressed(EMouseButton::Right))
	{
		return;
	}
	glm::vec3 right = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
	glm::vec3 up = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

	glm::vec3 cameraDir(0.f);

	if (InputManager::IsKeyDown(EKeyCode::W))
	{
		cameraDir += forward;
	}
	if (InputManager::IsKeyDown(EKeyCode::S))
	{
		cameraDir -= forward;
	}
	if (InputManager::IsKeyDown(EKeyCode::A))
	{
		cameraDir -= right;
	}
	if (InputManager::IsKeyDown(EKeyCode::D))
	{
		cameraDir += right;
	}
	if (InputManager::IsKeyDown(EKeyCode::Q))
	{
		cameraDir -= up;
	}
	if (InputManager::IsKeyDown(EKeyCode::E))
	{
		cameraDir += up;
	}
	if (cameraDir != Vector3Math::ZERO)
	{
		// constexpr float maxSpeed = 5000.0F;
		constexpr float maxSpeed = 20.0F;
		this->m_blendValue = MathUtils::Clamp(this->m_blendValue + delta, 0.0F, 1.0F);
		float speed = maxSpeed * ApplyAccelerationCurve(this->m_blendValue);
		this->m_position += glm::normalize(cameraDir) * speed * delta;
	}
	else
	{
		this->m_blendValue = 0.0f;
	}
	glm::vec2 mouseAcceleration = InputManager::GetMouseAcceleration();
	if (mouseAcceleration != glm::vec2{0.0f})
	{
		constexpr float mouseLookSpeed = 3.0f;
		this->m_yaw += mouseAcceleration.x * mouseLookSpeed * delta;
		this->m_pitch += mouseAcceleration.y * mouseLookSpeed * delta;
	}
}

glm::mat4 Hush::EditorCamera::GetOrientationMatrix() const noexcept
{
	glm::quat pitchRotation = glm::angleAxis(this->m_pitch, glm::vec3{-1.f, 0.f, 0.f});
	glm::quat yawRotation = glm::angleAxis(this->m_yaw, glm::vec3{0.f, -1.f, 0.f});

	return glm::toMat4(yawRotation) * glm::toMat4(pitchRotation);
}

glm::vec3 Hush::EditorCamera::GetPosition() const noexcept
{
	return this->m_position;
}

float Hush::EditorCamera::ApplyAccelerationCurve(float blend)
{
	// From a custom asymmetrical sigmoidal curve, formula approximated by: https://mycurvefit.com/
	// Raw formula: y = 1.082116 + (0.02923327 - 1.082116)/(1 + (x/0.2473429)^3.32689)^0.5257619
	constexpr float offset = 1.082116f;
	constexpr float numerator = 0.02923327f - 1.082116f;
	constexpr float c = 0.2473429f;
	constexpr float b = 3.32689f;
	constexpr float m = 0.5257619f;
	float expVariantFraction = MathUtils::Pow(blend / c, b);
	return offset + (numerator / MathUtils::Pow(1.0f + expVariantFraction, m));
}

glm::mat4 Hush::EditorCamera::GetViewMatrix() const noexcept
{
	glm::mat4 cameraTranslation = glm::translate(glm::mat4(1.f), this->m_position);
	glm::mat4 cameraRotation = this->GetOrientationMatrix();
	glm::mat4 viewMatrix = cameraTranslation * cameraRotation;
	return glm::inverse(viewMatrix);
}
