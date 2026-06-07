#include "ScenePanel.hpp"
#include "Logger.hpp"
#include "Scene.hpp"
#include <imgui/imgui.h>
#include "components/EditorPanelComponents.hpp"
#include <algorithm>

constexpr ImGuiWindowFlags SCENE_PANEL_FLAGS =
	ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse;

/// Minimum panel dimension in pixels — avoids creating zero-sized textures.
constexpr uint32_t MIN_PANEL_DIMENSION = 1;

void Hush::ScenePanel::Init(Scene *activeScene) noexcept
{
	(void)activeScene;
	this->m_bridgeEntity = activeScene->CreateEntityWithKey("ScenePanel");
	ScenePanelSizeComp &panelSize = this->m_bridgeEntity.AddComponent<ScenePanelSizeComp>();
	// Initially set this to the min dimensions
	panelSize.size = {MIN_PANEL_DIMENSION, MIN_PANEL_DIMENSION};
	this->m_panelSizeRef = this->m_bridgeEntity.CreateComponentReference<ScenePanelSizeComp>();
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
		// The render texture is now created to match the panel size, so
		// we display it at 1:1 — no aspect-ratio fitting needed.
		ImVec2 imageSize(static_cast<float>(m_textureWidth), static_cast<float>(m_textureHeight));

		// The ImGui WebGPU backend expects a WGPUTextureView cast as ImTextureID
		auto texId = reinterpret_cast<ImTextureID>(m_sceneTextureView);
		ImGui::Image(texId, imageSize);
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
