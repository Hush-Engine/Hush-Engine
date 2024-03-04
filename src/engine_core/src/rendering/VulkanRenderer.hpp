/*! \file VulkanRenderer.hpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Vulkan implementation for rendering
*/

#pragma once

#include "Renderer.hpp"

#define VK_NO_PROTOTYPES
#include <vector>
#include <vulkan/vulkan.h>

namespace Hush
{
    class VulkanRenderer final : public IRenderer
    {
      public:
        /// @brief Creates a new vulkan renderer from a given window context
        /// @param windowContext opaque pointer to the window context
        VulkanRenderer(void *windowContext);

        ~VulkanRenderer() override;

        void CreateSwapChain(uint32_t width, uint32_t height) override;

      private:
        void InitVulkan();

        void DestroySwapChain();

      private:
        VkInstance m_vulkanInstance;
        VkPhysicalDevice m_vulkanPhysicalDevice;
        VkDebugUtilsMessengerEXT m_debugMessenger;
        VkDevice m_device;
        VkSurfaceKHR m_surface;

        VkSwapchainKHR m_swapChain;
        VkFormat m_swapchainImageFormat;
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;
        VkExtent2D m_swapChainExtent;
    };
} // namespace Hush
