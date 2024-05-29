#include "ContentPanel.hpp"
#include <imgui/imgui.h>

constexpr ImGuiWindowFlags CONTENT_PANEL_FLAGS = ImGuiViewportFlags_NoFocusOnAppearing;
void Hush::ContentPanel::OnRender() noexcept
{
    if (ImGui::Begin("Project", nullptr, CONTENT_PANEL_FLAGS))
    {
    
    }
    ImGui::End();
}