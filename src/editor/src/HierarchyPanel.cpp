#include "HierarchyPanel.hpp"
#include <imgui/imgui.h>
#include <Assertions.hpp>

constexpr ImGuiWindowFlags DOCK_BASE_FLAGS =
    ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

void Hush::HierarchyPanel::OnRender()
{
    ImGuiViewport *mainViewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowViewport(mainViewport->ID);
    if (ImGui::Begin("Hierarchy"))
    {
        ImGuiIO &io = ImGui::GetIO();
        (void)io;
        ImGui::Text("Camera");
        ImGui::Text("Directional Light");
    }
    ImGui::End();
}