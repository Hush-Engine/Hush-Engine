#include "TitleBarMenuPanel.hpp"
#include "utils/networking/NetworkUtils.hpp"
#include <imgui/imgui.h>

constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_MenuBar;

void Hush::TitleBarMenuPanel::OnRender() noexcept
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New Scene", "Ctrl+N"))
            {
            }
            if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
            {
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
        {
            if (ImGui::MenuItem("About Hush Engine"))
            {
                Networking::SystemOpenURL("https://hushengine.com/");
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }
}