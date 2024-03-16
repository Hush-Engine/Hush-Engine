#include "MainPanel.hpp"
#include <imgui.h>
#include <imgui_impl_vulkan.h>

Hush::MainPanel::MainPanel(const IRenderer& renderer)
{
}

Hush::MainPanel::~MainPanel()
{
    ImGui::DestroyContext(this->m_uiContext.get());
    ImGui_ImplVulkan_Shutdown();
}

void Hush::MainPanel::OnRenderPass()
{
	//First, make the menu bar
    ImGui::BeginMainMenuBar();
}
