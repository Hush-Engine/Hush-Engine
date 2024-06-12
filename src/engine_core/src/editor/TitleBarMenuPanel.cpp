#include "TitleBarMenuPanel.hpp"
#include "utils/networking/NetworkUtils.hpp"
#include <imgui/imgui.h>
#include "UI.hpp"

constexpr ImGuiWindowFlags PANEL_FLAGS = ImGuiWindowFlags_MenuBar;

void Hush::TitleBarMenuPanel::OnRender() noexcept
{
    if (ImGui::BeginMainMenuBar())
    {
        FileMenuOptions();
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
    // #TODO: Also render the play options here
}

void Hush::TitleBarMenuPanel::FileMenuOptions()
{    
    if (!ImGui::BeginMenu("File"))
    {
        return;
    }

    if (ImGui::MenuItem("New Scene", "Ctrl+N"))
    {
    }
    if (ImGui::MenuItem("Open Scene", "Ctrl+O"))
    {
    }
    if (ImGui::MenuItem("Save", "Ctrl+S"))
    {
    
    }
    if (ImGui::MenuItem("Save Scene As...", "Ctrl+Shift+S"))
    {
    }

    if (ImGui::BeginMenu("Settings")) 
    {
        if (ImGui::MenuItem("Editor Settings"))
        {
        
        }
        if (ImGui::MenuItem("Project Settings"))
        {

        }
        ImGui::EndMenu();
    }

    ImGui::EndMenu();
}
