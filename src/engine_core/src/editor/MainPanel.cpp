#include "MainPanel.hpp"
#include <imgui.h>
#include <imgui_impl_vulkan.h>

Hush::MainPanel::MainPanel(const IRenderer &renderer) noexcept
{
    (void)renderer;
}

Hush::MainPanel::~MainPanel()
{
    ImGui::DestroyContext(this->m_uiContext);
    ImGui_ImplVulkan_Shutdown();
}

void Hush::MainPanel::OnRenderPass()
{
    // First, make the menu bar
    ImGui::BeginMainMenuBar();
}
