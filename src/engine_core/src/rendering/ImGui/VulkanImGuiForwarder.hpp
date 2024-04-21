/*! \file VulkanImGuiForwarder.hpp
    \author Leonidas Gonzalez
    \date 2024-03-16
    \brief Initializes the ImGui context with Vulkan specific implementations
*/

#pragma once
#include "../VulkanRenderer.hpp"
#include "IImGuiForwarder.hpp"
#include <imgui_impl_vulkan.h>

namespace Hush
{
    class VulkanImGuiForwarder : public IImGuiForwarder
    {
      public:
        void SetupImGui(const IRenderer &renderer) override;

      private:
        bool IsCorrectRendererType(const IRenderer &renderer);

        [[nodiscard]] ImGui_ImplVulkan_InitInfo CreateInitData(const VulkanRenderer &vulkanRenderer) const noexcept;

        VkDescriptorPool CreateImGuiPool(VkDevice device) const noexcept;
    };
} // namespace Hush
