#include "ScenePanel.hpp"
#include <imgui/imgui.h>

constexpr ImGuiWindowFlags SCENE_PANEL_FLAGS = ImGuiWindowFlags_NoScrollbar;

void Hush::ScenePanel::OnRender() noexcept
{
    ImGui::SetNextWindowBgAlpha(0.0f);
    ImGui::Begin("Scene", nullptr, SCENE_PANEL_FLAGS);
    ImGui::End();
} 