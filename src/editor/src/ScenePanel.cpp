#include "ScenePanel.hpp"
#include <imgui/imgui.h>

constexpr ImGuiWindowFlags SCENE_PANEL_FLAGS = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground;

void Hush::ScenePanel::OnRender() noexcept
{
	ImGui::Begin("Scene", nullptr, SCENE_PANEL_FLAGS);
	ImGui::End();
}