#include "ScenePanel.hpp"
#include "Components/GlobalKeys.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "InputManager.hpp"
#include "Logger.hpp"
#include "Scene.hpp"
#include "Shared/EditorCamera.hpp"
#include "components/EditorInfo.hpp"
#include "components/EditorPanelComponents.hpp"
#include "definitions/KeyCode.hpp"
#include "imguizmo/ImGuizmo.h"
#include <imgui/imgui.h>
#include <algorithm>

constexpr ImGuiWindowFlags SCENE_PANEL_FLAGS =
	ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse;

/// Minimum panel dimension in pixels — avoids creating zero-sized textures.
constexpr uint32_t MIN_PANEL_DIMENSION = 1;

void Hush::ScenePanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;

	this->m_bridgeEntity = activeScene->CreateEntityWithKey("ScenePanel");
	ScenePanelSizeComp &panelSize = this->m_bridgeEntity.AddComponent<ScenePanelSizeComp>();
	panelSize.size = {MIN_PANEL_DIMENSION, MIN_PANEL_DIMENSION};
	this->m_panelSizeRef = this->m_bridgeEntity.CreateComponentReference<ScenePanelSizeComp>();

	Entity editorInfoEntity = activeScene->CreateEntityWithKey(ENGINE_MANAGER);
	this->m_editorInfoRef = editorInfoEntity.CreateComponentReference<EditorInfo>();

	activeScene->CreateQuery<EditorCamera>().Each([this]([[maybe_unused]]
														 Entity &entity,
														 EditorCamera &camRef) { this->m_editorCamera = &camRef; });
}

glm::u32vec2 Hush::ScenePanel::GetPanelSize() const noexcept
{
	return *this->m_panelSizeRef.GetData<glm::u32vec2>();
}

void Hush::ScenePanel::OnRender(float deltaTime) noexcept
{
	auto *panelSize = this->m_panelSizeRef.GetData<ScenePanelSizeComp>();
	(void)deltaTime;

	ImGui::Begin("Scene", nullptr, SCENE_PANEL_FLAGS);

	auto *editorInfo = this->m_editorInfoRef.GetData<EditorInfo>();
	editorInfo->isMouseOnScene = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);

	// ── Track the panel's content region size every frame ────────────
	ImVec2 availSize = ImGui::GetContentRegionAvail();

	// DO NOT CAST THIS TO AN UNSIGNED INTEGER BECAUSE IT WILL UNDERFLOW
	const float clampedWidth = std::max(availSize.x, static_cast<float>(MIN_PANEL_DIMENSION));
	const float clampedHeight = std::max(availSize.y, static_cast<float>(MIN_PANEL_DIMENSION));
	auto newWidth = static_cast<uint32_t>(clampedWidth);
	auto newHeight = static_cast<uint32_t>(clampedHeight);

	if (newWidth != panelSize->size.x || newHeight != panelSize->size.y)
	{
		panelSize->size = {newWidth, newHeight};
		m_resized = true;
		this->m_bridgeEntity.NotifyComponentModifiedRaw(this->m_panelSizeRef.GetComponentId());
		LogFormat(ELogLevel::Info, "Size changed to ({}, {})", newWidth, newHeight);
	}

	// ── Display the scene texture ───────────────────────────────────
	if (m_sceneTextureView != nullptr)
	{
		ImVec2 imageSize(static_cast<float>(m_textureWidth), static_cast<float>(m_textureHeight));
		ImVec2 imagePos = ImGui::GetCursorScreenPos();

		auto texId = reinterpret_cast<ImTextureID>(m_sceneTextureView);
		ImGui::Image(texId, imageSize);

		this->RenderGizmo(imagePos, imageSize);
	}
	else
	{
		ImGui::TextDisabled("No scene texture available");
	}

	ImGui::End();
}

void Hush::ScenePanel::SetSceneTextureView(void *nativeTextureView, uint32_t width, uint32_t height) noexcept
{
	m_sceneTextureView = nativeTextureView;
	m_textureWidth = width;
	m_textureHeight = height;
}

void Hush::ScenePanel::SetGizmoTarget(Entity::EntityId entityId) noexcept
{
	m_gizmoTargetId = entityId;
}

void Hush::ScenePanel::SetGizmoOperation(ImGuizmo::OPERATION op) noexcept
{
	m_currentGizmoOp = op;
}

float GetSnapValueForOp(ImGuizmo::OPERATION operation)
{
	switch (operation)
	{
	case ImGuizmo::TRANSLATE:
		return 1.f;
	case ImGuizmo::ROTATE:
		return 15.f;
	case ImGuizmo::SCALE:
		return 5.f;
	default:
		return 0;
	}
}

void Hush::ScenePanel::RenderGizmo(const ImVec2 &imagePos, const ImVec2 &imageSize)
{
	if (m_gizmoTargetId == Entity::INVALID_ENTITY_ID || m_editorCamera == nullptr)
	{
		return;
	}

	std::optional<Entity> optEntity = this->m_activeScene->EntityFromId(m_gizmoTargetId);
	if (!optEntity.has_value() || !optEntity->IsValid())
	{
		return;
	}
	Entity &entity = optEntity.value();

	auto *editorInfo = this->m_editorInfoRef.GetData<EditorInfo>();
	if (editorInfo->currentState == EEditorState::None)
	{
		if (InputManager::IsKeyDownThisFrame(EKeyCode::R))
		{
			m_currentGizmoOp = ImGuizmo::ROTATE;
		}
		if (InputManager::IsKeyDownThisFrame(EKeyCode::T))
		{
			m_currentGizmoOp = ImGuizmo::TRANSLATE;
		}
		if (InputManager::IsKeyDownThisFrame(EKeyCode::S))
		{
			m_currentGizmoOp = ImGuizmo::SCALE;
		}
	}

	WorldTransform *worldXform = entity.GetComponent<WorldTransform>();
	LocalTransform *localXform = entity.GetComponent<LocalTransform>();
	if (worldXform == nullptr || localXform == nullptr)
	{
		return;
	}

	const glm::mat4 viewMat = m_editorCamera->GetViewMatrix();
	const glm::mat4 projMat = m_editorCamera->GetProjectionMatrix();

	ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
	ImGuizmo::SetRect(imagePos.x, imagePos.y, imageSize.x, imageSize.y);

	glm::mat4 worldMatrix = worldXform->GetTransformationMatrix();

	float snapBacking = 0;
	float *snap = nullptr;

	if (InputManager::IsKeyDown(EKeyCode::LCtrl) || InputManager::IsKeyDown(EKeyCode::RCtrl))
	{
		// Snap value depends on the operation to be done
		snap = &snapBacking;
		*snap = GetSnapValueForOp(this->m_currentGizmoOp);
	}

	bool isManipulating = ImGuizmo::Manipulate(
		reinterpret_cast<const float *>(&viewMat), reinterpret_cast<const float *>(&projMat), m_currentGizmoOp,
		ImGuizmo::MODE::LOCAL, reinterpret_cast<float *>(&worldMatrix), nullptr, snap);

	if (!isManipulating || !ImGuizmo::IsUsing())
	{
		return;
	}

	glm::mat4 newLocalMatrix = worldMatrix;
	const Entity parent = entity.GetParent();
	if (parent.IsValid())
	{
		const WorldTransform *parentWorldXform = parent.GetComponent<WorldTransform>();
		const glm::mat4 parentWorldMatrix = parentWorldXform->GetTransformationMatrix();
		newLocalMatrix = glm::inverse(parentWorldMatrix) * worldMatrix;
	}

	localXform->SetTransformationMatrix(newLocalMatrix);
	worldXform->SetTransformationMatrix(worldMatrix);
}
