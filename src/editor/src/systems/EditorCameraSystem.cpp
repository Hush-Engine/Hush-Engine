#include "EditorCameraSystem.hpp"
#include "Assertions.hpp"
#include "Components/GlobalKeys.hpp"
#include "InputManager.hpp"
#include "MathUtils.hpp"
#include "Profiling.hpp"
#include "Renderer.hpp"
#include "Scene.hpp"
#include "../components/EditorInfo.hpp"
#include "Shared/EditorCamera.hpp"
#include <glm/ext/vector_float3.hpp>

constexpr float CAM_PITCH_MIN = -89.5f * Hush::MathUtils::DEG_TO_RAD;
constexpr float CAM_PITCH_MAX = 89.5f * Hush::MathUtils::DEG_TO_RAD;

void Hush::EditorCameraSystem::Init()
{
	// There should only ever be ONE EditorCamera component in the active scene
	Entity editorCamEntity = this->GetScene().CreateEntityWithKey(EDITOR_CAMERA);
	this->m_editorCameraRef = editorCamEntity.CreateComponentReference<EditorCamera>();

	Entity editorCamEntity = this->GetScene().CreateEntityWithKey(EDITOR_CAMERA);
	this->m_editorCameraRef = editorCamEntity.CreateComponentReference<EditorCamera>();

	// There should only ever be ONE EditorInfo component in the active scene
	Entity managerEntity = this->GetScene().CreateEntityWithKey(ENGINE_MANAGER);
	this->m_editorInfoRef = managerEntity.CreateComponentReference<EditorInfo>();
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
	auto *editorCamera = this->m_editorCameraRef.GetData<EditorCamera>();
	HUSH_ASSERT(editorCamera != nullptr, "Editor camera component should never be null if the HushEditor is running!");
	auto *editorInfo = this->m_editorInfoRef.GetData<EditorInfo>();
	HUSH_ASSERT(editorInfo != nullptr, "Editor info component should never be null if the HushEditor is running!");

	if (editorCamera == nullptr)
	{
		return;
	}

	glm::mat4 viewMatrix = editorCamera->GetViewMatrix();
	glm::vec3 forward = -glm::vec3(viewMatrix[0][2], viewMatrix[1][2], viewMatrix[2][2]);
	glm::vec3 right = glm::vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
	glm::vec3 up = glm::vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);

	glm::vec3 &positionRef = editorCamera->GetPosition();
	if (InputManager::GetMouseScrollAcceleration().y != 0.0F && editorInfo->isMouseOnScene)
	{
		constexpr float zoomSpeed = 10.F;
		positionRef += forward * InputManager::GetMouseScrollAcceleration().y * zoomSpeed * delta;
	}

	if (!InputManager::GetMouseButtonPressed(EMouseButton::Right))
	{
		// Only reset the state if we controlled the current one
		if (editorInfo->currentState == EEditorState::FreeLook)
		{
			editorInfo->currentState = EEditorState::None;
		}
		return;
	}
	editorInfo->currentState = EEditorState::FreeLook;

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
		float &yaw = editorCamera->GetYaw();
		float &pitch = editorCamera->GetPitch();
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

void Hush::EditorCameraSystem::OnFixedUpdate([[maybe_unused]] float delta)
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
