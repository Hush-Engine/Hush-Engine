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

        VulkanRenderer(const VulkanRenderer &) = delete;
        VulkanRenderer &operator=(const VulkanRenderer &) = delete;

        VulkanRenderer(VulkanRenderer &&rhs) noexcept;
        VulkanRenderer &operator=(VulkanRenderer &&rhs) noexcept;

        ~VulkanRenderer() override;

        void CreateSwapChain(uint32_t width, uint32_t height) override;

        /* CONSTANT GETTERS */

        [[nodiscard]] VkInstance GetVulkanInstance() const noexcept;

        [[nodiscard]] VkDevice GetVulkanDevice() const noexcept;
        
        [[nodiscard]] VkPhysicalDevice GetVulkanPhysicalDevice() const noexcept;

        [[nodiscard]] VkQueue GetVulkanQueue() const noexcept;
        
      private:
        void InitVulkan();

        void DestroySwapChain();

        void *m_windowContext;
        VkInstance m_vulkanInstance = nullptr;
        VkPhysicalDevice m_vulkanPhysicalDevice = nullptr;
        VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
        VkDevice m_device = nullptr;
        VkSurfaceKHR m_surface{};

        VkSwapchainKHR m_swapChain{};
        VkFormat m_swapchainImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
        std::vector<VkImage> m_swapchainImages{};
        std::vector<VkImageView> m_swapchainImageViews{};
        VkExtent2D m_swapChainExtent{};
    };
} // namespace Hush
