/*! \file VulkanImGuiForwarder.hpp
    \author Leonidas Gonzalez
    \date 2024-03-16
    \brief Initializes the ImGui context with Vulkan specific implementations
*/

#pragma once
#include "rendering/ImGui/IImGuiForwarder.hpp"
#include <imgui/backends/imgui_impl_vulkan.h>

namespace Hush
{
    class VulkanRenderer;
    class VulkanImGuiForwarder : public IImGuiForwarder
    {
      public:
        void SetupImGui(IRenderer* renderer) override;

        void NewFrame() override;

        void HandleEvent(const SDL_Event *event) noexcept override;

        void RenderFrame(VkCommandBuffer cmd);

      private:
        [[nodiscard]] ImGui_ImplVulkan_InitInfo CreateInitData(VulkanRenderer* vulkanRenderer) const noexcept;

        VkDescriptorPool CreateImGuiPool(VkDevice device) const noexcept;
    };
} // namespace Hush
