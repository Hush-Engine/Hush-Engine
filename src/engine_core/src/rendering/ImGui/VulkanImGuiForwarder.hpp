/*! \file VulkanImGuiForwarder.hpp
    \author Leonidas Gonzalez
    \date 2024-03-16
    \brief Initializes the ImGui context with Vulkan specific implementations
*/

#pragma once
#include "../Vulkan/VulkanRenderer.hpp"
#include "IImGuiForwarder.hpp"
#include <imgui/backends/imgui_impl_vulkan.h>

namespace Hush
{
    class VulkanImGuiForwarder : public IImGuiForwarder
    {
      public:
        void SetupImGui(IRenderer* renderer) override;

      private:
        [[nodiscard]] ImGui_ImplVulkan_InitInfo CreateInitData(VulkanRenderer* vulkanRenderer) const noexcept;

        VkDescriptorPool CreateImGuiPool(VkDevice device) const noexcept;
    };
} // namespace Hush
