#include "EditorCameraSystem.hpp"
#include "MathUtils.hpp"
#include "Profiling.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "WindowManager.hpp"
#include "../UIUtils.hpp"
#include <glm/ext/vector_float3.hpp>

constexpr float CAM_PITCH_MIN = -89.5f * Hush::MathUtils::DEG_TO_RAD;
constexpr float CAM_PITCH_MAX = 89.5f * Hush::MathUtils::DEG_TO_RAD;

void Hush::EditorCameraSystem::Init()
{
	// There should only ever be ONE EditorCamera component in the active scene
	this->GetScene().CreateQuery<EditorCamera>().Each(
		[this](Entity &entity, [[maybe_unused]]
							   EditorCamera &camRef) { this->m_editorCameraEntity = std::move(entity); });

	// There should only ever be ONE EditorInfo component in the active scene
	this->GetScene().CreateQuery<EditorInfo>().Each(
		[this](Entity &entity, [[maybe_unused]]
							   EditorInfo &infoRef) { this->m_editorInfoEntity = std::move(entity); });
}

void Hush::EditorCameraSystem::OnShutdown()
{
}

void Hush::EditorCameraSystem::OnUpdate(float delta)
{
	ZoneScoped;
	// We need to retrieve the camera and editor info references every frame because the scene might have been reloaded,
	// which destroys all existing entities and components.  This is a bit hacky but it avoids having to add a more
	// complex event system just for this.
	m_editorCamera = m_editorCameraEntity.GetComponent<EditorCamera>();
	m_editorInfo = m_editorInfoEntity.GetComponent<EditorInfo>();
	if (this->m_editorCamera == nullptr || this->m_editorInfo == nullptr)
	{
		return;
	}

	glm::mat4 viewMatrix = this->m_editorCamera->GetViewMatrix();
	glm::vec3 forward = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
	glm::vec3 &positionRef = this->m_editorCamera->GetPosition();
	if (InputManager::GetMouseScrollAcceleration().y != 0.0F && UIUtils::IsMouseInScene())
	{
		constexpr float zoomSpeed = 100.F;
		positionRef += forward * InputManager::GetMouseScrollAcceleration().y * zoomSpeed * delta;
	}

	if (!InputManager::GetMouseButtonPressed(EMouseButton::Right))
	{
		// Only reset the state if we controlled the current one
		if (this->m_editorInfo->currentState == EEditorState::FreeLook)
		{
			this->m_editorInfo->currentState = EEditorState::None;
		}
		return;
	}
	this->m_editorInfo->currentState = EEditorState::FreeLook;

	glm::vec3 right = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
	glm::vec3 up = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

	glm::vec3 cameraDir(0.F);

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
		positionRef += glm::normalize(cameraDir) * speed * delta;
	}
	else
	{
		this->m_blendValue = 0.0F;
	}
	glm::vec2 mouseAcceleration = InputManager::GetMouseAcceleration();
	if (mouseAcceleration != glm::vec2{0.0F})
	{
		constexpr float mouseLookSpeed = 3.0F;
		float &yaw = this->m_editorCamera->GetYaw();
		float &pitch = this->m_editorCamera->GetPitch();
		yaw += mouseAcceleration.x * mouseLookSpeed * delta;
		pitch = MathUtils::Clamp(pitch + (mouseAcceleration.y * mouseLookSpeed * delta), CAM_PITCH_MIN, CAM_PITCH_MAX);
	}
}

// NOLINTBEGIN
float Hush::EditorCameraSystem::ApplyAccelerationCurve(float blend)
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
// NOLINTEND

void Hush::EditorCameraSystem::OnFixedUpdate(float delta)
{
}

void Hush::EditorCameraSystem::OnRender()
{
}

void Hush::EditorCameraSystem::OnPreRender()
{
}

void Hush::EditorCameraSystem::OnPostRender()
{
}

std::string_view Hush::EditorCameraSystem::GetName() const
{
	return "EditorCameraSystem";
}
