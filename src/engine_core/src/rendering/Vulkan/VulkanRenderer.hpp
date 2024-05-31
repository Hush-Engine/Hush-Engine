/*! \file VulkanRenderer.hpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Vulkan implementation for rendering
*/

#pragma once
#include "rendering/Renderer.hpp"
#include "utils/Assertions.hpp"
#include <magic_enum.hpp>

#define VK_NO_PROTOTYPES

#include "FrameData.hpp"
#include "VkTypes.hpp"
#include "VulkanDeletionQueue.hpp"
#include "rendering/ImGui/IImGuiForwarder.hpp"
#include "vk_mem_alloc.hpp"
#include <VkBootstrap.h>
#include <array>
#include <functional>
#include <vector>
#include <vulkan/vulkan.h>

///@brief Double frame buffering, allows for the GPU and CPU to work in parallel. NOTE: increase to 3 if experiencing
/// jittery framerates
constexpr uint32_t FRAME_OVERLAP = 2;

constexpr uint32_t VK_OPERATION_TIMEOUT_NS = 1'000'000'000; // This is one second, trust me (1E-9)

namespace Hush
{

    class VulkanRenderer final : public IRenderer
    {
      public:
        static PFN_vkVoidFunction CustomVulkanFunctionLoader(const char *functionName, void *userData);
        /// @brief Creates a new vulkan renderer from a given window context
        /// @param windowContext opaque pointer to the window context
        VulkanRenderer(void *windowContext);

        VulkanRenderer(const VulkanRenderer &) = delete;
        VulkanRenderer &operator=(const VulkanRenderer &) = delete;

        VulkanRenderer(VulkanRenderer &&rhs) noexcept;
        VulkanRenderer &operator=(VulkanRenderer &&rhs) noexcept;

        ~VulkanRenderer() override;

        void CreateSwapChain(uint32_t width, uint32_t height) override;

        void InitRendering() override;

        void InitializeCommands() noexcept;

        void InitImGui() override;

        void Draw() override;

        void NewUIFrame() const noexcept override;

        void HandleEvent(const SDL_Event *event) noexcept override;

        void Dispose();

        void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function) noexcept;

        FrameData &GetCurrentFrame() noexcept;

        FrameData &GetLastFrame() noexcept;

        /* CONSTANT GETTERS */

        [[nodiscard]] VkInstance GetVulkanInstance() const noexcept;

        [[nodiscard]] VkDevice GetVulkanDevice() const noexcept;

        [[nodiscard]] VkPhysicalDevice GetVulkanPhysicalDevice() const noexcept;

        [[nodiscard]] VkQueue GetGraphicsQueue() const noexcept;

        VkFormat *GetSwapchainImageFormat() noexcept;

        [[nodiscard]] void *GetWindowContext() const noexcept override;

      private:
        void Configure(vkb::Instance vkbInstance);

        void CreateSyncObjects();

        void DestroySwapChain();

        VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signalSemaphoreInfo,
                                 VkSemaphoreSubmitInfo *waitSemaphoreInfo);

        void LoadDebugMessenger();

        static uint32_t LogDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

        void InitVmaAllocator();

        void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

        void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize,
                              VkExtent2D dstSize);

        void *m_windowContext;
        // TODO: Send all of these to a custom struct holding the pointers
        VkInstance m_vulkanInstance = nullptr;
        VkPhysicalDevice m_vulkanPhysicalDevice = nullptr;
        VkDebugUtilsMessengerEXT m_debugMessenger = nullptr;
        VkDevice m_device = nullptr;
        VkSurfaceKHR m_surface{};
        VkQueue m_graphicsQueue = nullptr;
        VkFence m_immediateFence = nullptr;
        VkCommandBuffer m_immediateCommandBuffer = nullptr;
        VkCommandPool m_immediateCommandPool = nullptr;
        uint32_t m_graphicsQueueFamily = 0u;

        VkSwapchainKHR m_swapChain{};
        VkFormat m_swapchainImageFormat = VkFormat::VK_FORMAT_UNDEFINED;
        std::vector<VkImage> m_swapchainImages{};
        std::vector<VkImageView> m_swapchainImageViews{};
        VkExtent2D m_swapChainExtent{};
        uint32_t m_width = 0u;
        uint32_t m_height = 0u;
        // draw resources
        AllocatedImage m_drawImage{};

        // Frame related data
        std::array<FrameData, FRAME_OVERLAP> m_frames{};
        // Frame counter
        //(This should run fine for like, 414 days at 60 fps, and 69 days at like 360 fps)
        int m_frameNumber = 0;
        std::unique_ptr<IImGuiForwarder> m_uiForwarder = nullptr;

        VulkanDeletionQueue m_mainDeletionQueue{};
        VmaAllocator m_allocator = nullptr; // vma lib allocator
    };
} // namespace Hush
