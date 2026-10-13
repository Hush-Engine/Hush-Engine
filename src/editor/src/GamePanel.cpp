#include "GamePanel.hpp"
#include "Logger.hpp"
#include "components/EditorPanelComponents.hpp"
#include <imgui/imgui.h>
#include <algorithm>

constexpr ImGuiWindowFlags GAME_PANEL_FLAGS =
	ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse;

/// Minimum panel dimension in pixels — avoids creating zero-sized textures.
constexpr uint32_t MIN_PANEL_DIMENSION = 1;

void Hush::GamePanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;

	this->m_bridgeEntity = activeScene->CreateEntityWithKey("GamePanel");
	GamePanelSizeComp &panelSize = this->m_bridgeEntity.AddComponent<GamePanelSizeComp>();
	panelSize.size = {MIN_PANEL_DIMENSION, MIN_PANEL_DIMENSION};
	this->m_panelSizeRef = this->m_bridgeEntity.CreateComponentReference<GamePanelSizeComp>();
}

glm::u32vec2 Hush::GamePanel::GetPanelSize() const noexcept
{
	return *this->m_panelSizeRef.GetData<glm::u32vec2>();
}

void Hush::GamePanel::OnRender(float deltaTime) noexcept
{
	auto *panelSize = this->m_panelSizeRef.GetData<GamePanelSizeComp>();
	(void)deltaTime;

	ImGui::Begin("Game", nullptr, GAME_PANEL_FLAGS);

	// Capture the content-area origin in window space before any content is drawn.
	// This is the offset needed to convert window-space mouse coords to viewport-relative NDC.
	ImVec2 cursorScreenPos = ImGui::GetCursorScreenPos();
	glm::vec2 newPos = {cursorScreenPos.x, cursorScreenPos.y};
	if (newPos != panelSize->position)
	{
		panelSize->position = newPos;
		this->m_bridgeEntity.NotifyComponentModifiedRaw(this->m_panelSizeRef.GetComponentId());
	}

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

	// ── Display the game view texture ───────────────────────────────
	if (m_gameTextureView != nullptr)
	{
		ImVec2 imageSize(static_cast<float>(m_textureWidth), static_cast<float>(m_textureHeight));

		auto texId = reinterpret_cast<ImTextureID>(m_gameTextureView);
		ImGui::Image(texId, imageSize);
	}
	else
	{
		ImGui::TextDisabled("No game view available");
	}

	ImGui::End();
}

void Hush::GamePanel::SetGameTextureView(void *nativeTextureView, uint32_t width, uint32_t height) noexcept
{
	m_gameTextureView = nativeTextureView;
	m_textureWidth = width;
	m_textureHeight = height;
}
