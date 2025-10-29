#include "ScenePanel.hpp"
#include "Scene.hpp"
#include <imgui/imgui.h>

constexpr ImGuiWindowFlags SCENE_PANEL_FLAGS = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;

void Hush::ScenePanel::Init(Scene *activeScene) noexcept
{
	(void)activeScene;
}

void Hush::ScenePanel::OnRender(float deltaTime) noexcept
{
	ImGui::Begin("Scene", nullptr, SCENE_PANEL_FLAGS);
	ImGui::End();
}
