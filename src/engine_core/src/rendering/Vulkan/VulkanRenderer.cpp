/*! \file VulkanRenderer.cpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Vulkan implementation for rendering
*/

#include "VulkanRenderer.hpp"
#include "log/Logger.hpp"
#include "utils/Platform.hpp"

#include <SDL2/SDL_vulkan.h>

#if HUSH_PLATFORM_WIN
#define VK_USE_PLATFORM_WIN32_KHR
#elif HUSH_PLATFORM_LINUX
#endif
#define VOLK_IMPLEMENTATION
#include "VkUtilsFactory.hpp"
#include "utils/Assertions.hpp"
#include <utils/typeutils/TypeUtils.hpp>
#include <volk.h>

Hush::VulkanRenderer::VulkanRenderer(void *windowContext)
    : Hush::IRenderer(windowContext), m_windowContext(windowContext)
{
    LogTrace("Initializing Vulkan");

    // Validate the window context
    bool isInstanceOfSdl = TypeUtils::IsInstanceOf<SDL_Window *, void *>(windowContext);
    HUSH_ASSERT(isInstanceOfSdl, "Window pointer is not of type SDL_Window*");

    VkResult rc = volkInitialize();
    HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Error initializing Vulkan renderer, error: {}!", static_cast<int>(rc));

    // Simplify vulkan creation with VkBootstrap
    // TODO: we might use the vulkan API without another dependency in the future???
    vkb::InstanceBuilder builder{};
    vkb::Result<vkb::Instance> instanceResult =
        builder.set_app_name("Hush Engine")
            .request_validation_layers(true)
            // TODO: We might use a lower version for some platforms such as Android
            .require_api_version(1, 3, 0)
            .build();

    HUSH_ASSERT(instanceResult, "Cannot load instance: {}", instanceResult.error().message());

    vkb::Instance vkbInstance = instanceResult.value();

    LogTrace("Got vulkan instance");

    this->m_vulkanInstance = vkbInstance.instance;
    this->m_debugMessenger = vkbInstance.debug_messenger;
    volkLoadInstance(this->m_vulkanInstance);
    auto *sdlWindowContext = static_cast<SDL_Window *>(windowContext);
    // Creates the Vulkan Surface from the SDL window context
    SDL_bool createSurfaceResult = SDL_Vulkan_CreateSurface(sdlWindowContext, this->m_vulkanInstance, &this->m_surface);
    HUSH_ASSERT(createSurfaceResult == SDL_TRUE, "Cannot create vulkan surface, error: {}!", SDL_GetError());
    LogTrace("Initialized vulkan surface");
    // Configure our renderer with the proper extensions / device properties, etc.
    this->Configure(vkbInstance);
}

Hush::VulkanRenderer::VulkanRenderer(VulkanRenderer &&rhs) noexcept
    : IRenderer(nullptr), m_windowContext(rhs.m_windowContext), m_vulkanInstance(rhs.m_vulkanInstance),
      m_vulkanPhysicalDevice(rhs.m_vulkanPhysicalDevice), m_debugMessenger(rhs.m_debugMessenger),
      m_device(rhs.m_device), m_surface(rhs.m_surface), m_swapChain(rhs.m_swapChain),
      m_swapchainImageFormat(rhs.m_swapchainImageFormat), m_swapchainImages(std::move(rhs.m_swapchainImages)),
      m_swapchainImageViews(std::move(rhs.m_swapchainImageViews)), m_swapChainExtent(rhs.m_swapChainExtent)
{
    rhs.m_vulkanInstance = nullptr;
    rhs.m_vulkanPhysicalDevice = nullptr;
    rhs.m_debugMessenger = nullptr;
    rhs.m_device = nullptr;
    rhs.m_surface = nullptr;
    rhs.m_swapChain = nullptr;
    rhs.m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
    rhs.m_swapchainImages.clear();
    rhs.m_swapchainImageViews.clear();
    rhs.m_swapChainExtent = VkExtent2D{};
}

Hush::VulkanRenderer &Hush::VulkanRenderer::operator=(VulkanRenderer &&rhs) noexcept
{
    if (this != &rhs)
    {
        this->m_windowContext = rhs.m_windowContext;
        this->m_vulkanInstance = rhs.m_vulkanInstance;
        this->m_vulkanPhysicalDevice = rhs.m_vulkanPhysicalDevice;
        this->m_debugMessenger = rhs.m_debugMessenger;
        this->m_device = rhs.m_device;
        this->m_surface = rhs.m_surface;
        this->m_swapChain = rhs.m_swapChain;
        this->m_swapchainImageFormat = rhs.m_swapchainImageFormat;
        this->m_swapchainImages = std::move(rhs.m_swapchainImages);
        this->m_swapchainImageViews = std::move(rhs.m_swapchainImageViews);
        this->m_swapChainExtent = rhs.m_swapChainExtent;

        rhs.m_vulkanInstance = nullptr;
        rhs.m_vulkanPhysicalDevice = nullptr;
        rhs.m_debugMessenger = nullptr;
        rhs.m_device = nullptr;
        rhs.m_surface = nullptr;
        rhs.m_swapChain = nullptr;
        rhs.m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
        rhs.m_swapchainImages.clear();
        rhs.m_swapchainImageViews.clear();
        rhs.m_swapChainExtent = VkExtent2D{};
    }

    return *this;
}

Hush::VulkanRenderer::~VulkanRenderer()
{
    this->Dispose();
}

// Called on resize and window init
void Hush::VulkanRenderer::CreateSwapChain(uint32_t width, uint32_t height)
{
    vkb::SwapchainBuilder swapchainBuilder{m_vulkanPhysicalDevice, m_device, m_surface};

    this->m_swapchainImageFormat = VK_FORMAT_B8G8R8A8_UNORM;

    auto vkSurfaceFormat = VkSurfaceFormatKHR{};
    vkSurfaceFormat.format = this->m_swapchainImageFormat;
    vkSurfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

    vkb::Swapchain vkbSwapChain = swapchainBuilder.set_desired_format(vkSurfaceFormat)
                                      .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
                                      .set_desired_extent(width, height)
                                      .add_image_usage_flags(VK_IMAGE_USAGE_TRANSFER_DST_BIT)
                                      .build()
                                      .value();

    this->m_width = width;
    this->m_height = height;
    this->m_swapChainExtent = vkbSwapChain.extent;
    this->m_swapChain = vkbSwapChain.swapchain;
    this->m_swapchainImages = vkbSwapChain.get_images().value();
    this->m_swapchainImageViews = vkbSwapChain.get_image_views().value();
}

void Hush::VulkanRenderer::InitializeCommands() noexcept
{
    VkCommandPoolCreateInfo commandPoolInfo = VkUtilsFactory::CreateCommandPoolInfo(
        this->m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkResult rc{};

    //Initialize the immediate command structures
    rc = vkCreateCommandPool(this->m_device, &commandPoolInfo, nullptr, &this->m_immidiateCommandPool);
    HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Creating immediate command pool failed with code {}",
                static_cast<int>(rc));

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo = VkUtilsFactory::CreateCommandBufferAllocateInfo(this->m_immidiateCommandPool);
    rc = vkAllocateCommandBuffers(this->m_device, &cmdAllocInfo, &this->m_immidiateCommandBuffer);
    HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Allocating immidiate command buffers failed with code {}",
                static_cast<int>(rc));

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        FrameData currFrame = this->m_frames.at(i);
        rc = vkCreateCommandPool(this->m_device, &commandPoolInfo, nullptr, &currFrame.commandPool);
        HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Creating command pool failed with code: {}", static_cast<int>(rc));

        // allocate the default command buffer that we will use for rendering
        VkCommandBufferAllocateInfo cmdAllocInfo =
            VkUtilsFactory::CreateCommandBufferAllocateInfo(currFrame.commandPool);
        rc = vkAllocateCommandBuffers(this->m_device, &cmdAllocInfo, &currFrame.mainCommandBuffer);
        HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Allocating command buffers failed with code: {}",
                    static_cast<int>(rc));
    }
}

void Hush::VulkanRenderer::Draw()
{
    auto *sdlWindowContext = static_cast<SDL_Window *>(this->m_windowContext);
    // Skip rendering if the window is minimized
    uint32_t minimizedFlagResult = SDL_GetWindowFlags(sdlWindowContext) & SDL_WINDOW_MINIMIZED;
    if (minimizedFlagResult == 1u)
    {
        return;
    }
    FrameData currentFrame = this->GetCurrentFrame();
    const uint32_t fenceTargetCount = 1u;
    // Wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VkResult rc =
        vkWaitForFences(this->m_device, fenceTargetCount, &currentFrame.renderFence, VK_TRUE, VK_OPERATION_TIMEOUT_NS);
    HUSH_VK_ASSERT(rc, "Fence wait failed!");
    rc = vkResetFences(this->m_device, fenceTargetCount, &currentFrame.renderFence);
    HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Fence reset failed with code: {}", static_cast<int>(rc));

    // Request an image from the swapchain
    uint32_t swapchainImageIndex = 0u;
    rc = vkAcquireNextImageKHR(this->m_device, this->m_swapChain, VK_OPERATION_TIMEOUT_NS,
                               currentFrame.swapchainSemaphore, nullptr, &swapchainImageIndex);
    HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Image request from the swapchain failed with code: {}",
                static_cast<int>(rc));

    // Get the command buffer and reset it
    VkCommandBuffer cmd = currentFrame.mainCommandBuffer;
    // Reset the command buffer
    rc = vkResetCommandBuffer(cmd, 0u);
    HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Reset command buffer failed with code: {}", static_cast<int>(rc));

    // Begin recording
    VkCommandBufferBeginInfo cmdBeginInfo = VkUtilsFactory::CreateCommandBufferBeginInfo(
        VkCommandBufferUsageFlagBits::VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    rc = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
    HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Begin command buffer failed with code: {}", static_cast<int>(rc));

    /**** Set up dynamic rendering objects ****/

    // Create our color attachment
    VkClearValue clearValue = {};
    clearValue.color = {0.0f, 0.0f, 0.0f, 1.0f};
    VkRenderingAttachmentInfoKHR colorAttachmentInfo =
        VkUtilsFactory::CreateColorAttachmentInfo(this->m_swapchainImageViews.at(swapchainImageIndex), clearValue);

    // Init
    VkRenderingInfoKHR renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
    renderingInfo.renderArea.offset.x = 0;
    renderingInfo.renderArea.offset.y = 0;
    renderingInfo.renderArea.extent.width = this->m_width;
    renderingInfo.renderArea.extent.height = this->m_height;
    renderingInfo.layerCount = 1;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachments = &colorAttachmentInfo;

    // Begin dynamic rendering
    vkCmdBeginRenderingKHR(cmd, &renderingInfo);

    // Record your rendering commands here

    // End dynamic rendering
    vkCmdEndRenderingKHR(cmd);

    // End recording
    rc = vkEndCommandBuffer(cmd);
    HUSH_VK_ASSERT(rc, "End command buffer failed!");

    this->m_frameNumber++;
}

void Hush::VulkanRenderer::Begin()
{
}

void Hush::VulkanRenderer::Dispose()
{
    this->DestroySwapChain();
    if (this->m_device != nullptr)
    {
        vkDeviceWaitIdle(this->m_device);
        for (int i = 0; i < FRAME_OVERLAP; i++)
        {
            // Delete any command pools
            vkDestroyCommandPool(this->m_device, this->m_frames.at(i).commandPool, nullptr);
            // Destroy the sync objects
            vkDestroyFence(this->m_device, this->m_frames.at(i).renderFence, nullptr);
            vkDestroySemaphore(this->m_device, this->m_frames.at(i).renderSemaphore, nullptr);
            vkDestroySemaphore(this->m_device, this->m_frames.at(i).swapchainSemaphore, nullptr);
        }
        vkDestroyDevice(this->m_device, nullptr);
    }

    if (this->m_surface != nullptr)
    {
        vkDestroySurfaceKHR(this->m_vulkanInstance, this->m_surface, nullptr);
    }

    if (this->m_debugMessenger != nullptr)
    {
        vkb::destroy_debug_utils_messenger(this->m_vulkanInstance, this->m_debugMessenger, nullptr);
    }

    if (this->m_vulkanInstance != nullptr)
    {
        vkDestroyInstance(this->m_vulkanInstance, nullptr);
    }

    LogTrace("Vulkan resources destroyed");
}

void Hush::VulkanRenderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function) noexcept
{
    VkResult rc = vkResetFences(this->m_device, 1u, &this->m_immediateFence);
    HUSH_VK_ASSERT(rc, "Failed to reset immediate fence!");
    
    rc = vkResetCommandBuffer(this->m_immediateCommandBuffer, 0);
    HUSH_VK_ASSERT(rc, "Failed to reset immediate command buffer!");
    
    VkCommandBufferBeginInfo cmdBeginInfo = VkUtilsFactory::CreateCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    rc = vkBeginCommandBuffer(this->m_immediateCommandBuffer, &cmdBeginInfo);
    HUSH_VK_ASSERT(rc, "Failed to initialize immediate command buffer!");
    
    function(this->m_immediateCommandBuffer);

    rc = vkEndCommandBuffer(this->m_immediateCommandBuffer);
    HUSH_VK_ASSERT(rc, "Failed to end immediate command buffer!");

    VkCommandBufferSubmitInfo cmdSubmitInfo =
        VkUtilsFactory::CreateCommandBufferSubmitInfo(this->m_immediateCommandBuffer);
    VkSubmitInfo2 submit = this->SubmitInfo(&cmdSubmitInfo, nullptr, nullptr);

    rc = vkQueueSubmit2(this->m_graphicsQueue, 1u, &submit, this->m_immediateFence);
    HUSH_VK_ASSERT(rc, "Failed to submit graphics queue!");

    rc = vkWaitForFences(this->m_device, 1u, &this->m_immediateFence, VK_TRUE, 9999999999);
    HUSH_VK_ASSERT(rc, "Immediate fence timed out");
}

VkInstance Hush::VulkanRenderer::GetVulkanInstance() const noexcept
{
    return this->m_vulkanInstance;
}

VkDevice Hush::VulkanRenderer::GetVulkanDevice() const noexcept
{
    return this->m_device;
}

VkPhysicalDevice Hush::VulkanRenderer::GetVulkanPhysicalDevice() const noexcept
{
    return this->m_vulkanPhysicalDevice;
}

VkQueue Hush::VulkanRenderer::GetGraphicsQueue() const noexcept
{
    return this->m_graphicsQueue;
}

FrameData &Hush::VulkanRenderer::GetCurrentFrame() noexcept
{
    return this->m_frames.at(this->m_frameNumber % FRAME_OVERLAP);
}

FrameData &Hush::VulkanRenderer::GetLastFrame() noexcept
{
    // TODO: insert return statement here
    return this->m_frames.at((this->m_frameNumber - 1) % FRAME_OVERLAP);
}

void Hush::VulkanRenderer::Configure(vkb::Instance vkbInstance)
{
    // Get our features
    VkPhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.bufferDeviceAddress = VK_TRUE;
    vulkan12Features.descriptorIndexing = VK_TRUE;

    // Select our physical GPU
    vkb::PhysicalDeviceSelector selector{vkbInstance};
    vkb::PhysicalDevice vkbPhysicalDevice = selector.set_minimum_version(1, 3)
                                                .set_required_features_13(vulkan13Features)
                                                .set_surface(m_surface)
                                                .select()
                                                .value();

    // Get our virtual device based on the physical one
    vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice);

    vkb::Device vkbDevice = deviceBuilder.build().value();

    this->m_device = vkbDevice.device;
    this->m_vulkanPhysicalDevice = vkbDevice.physical_device;

    volkLoadDevice(this->m_device);

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(this->m_vulkanPhysicalDevice, &properties);

    // Get the queue
    vkb::Result<VkQueue> queueResult = vkbDevice.get_queue(vkb::QueueType::graphics);
    vkb::Result<uint32_t> queueIndexResult = vkbDevice.get_queue_index(vkb::QueueType::graphics);

    HUSH_ASSERT(queueResult, "Queue could not be gathered from Vulkan, error: {}!", queueResult.error().message());
    HUSH_ASSERT(queueResult, "Queue family could not be gathered from Vulkan, error: {}!",
                queueIndexResult.error().message());

    this->m_graphicsQueue = queueResult.value();
    this->m_graphicsQueueFamily = queueIndexResult.value();

    LogFormat(ELogLevel::Debug, "Device name: {}", properties.deviceName);
    LogFormat(ELogLevel::Debug, "API version: {}", properties.apiVersion);
}

VkFormat Hush::VulkanRenderer::GetSwapchainImageFormat() const noexcept
{
    return this->m_swapchainImageFormat;
}

void Hush::VulkanRenderer::CreateSyncObjects()
{
    // Create our sync objects and see if we were succesful
    VkUtilsFactory utilsFactory;
    VkFenceCreateInfo fenceInfo = utilsFactory.CreateFenceInfo(VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreInfo = utilsFactory.CreateSemaphoreInfo();

    VkResult rc{};
    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        rc = vkCreateFence(this->m_device, &fenceInfo, nullptr, &this->m_frames.at(i).renderFence);
        HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Creating fence failed with code: {}!", static_cast<int>(rc));

        // Create the semaphores
        rc = vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_frames.at(i).swapchainSemaphore);
        HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Creating swapchain semaphore failed with code: {}!",
                    static_cast<int>(rc));

        rc = vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_frames.at(i).renderSemaphore);
        HUSH_ASSERT(rc == VkResult::VK_SUCCESS, "Creating render semaphore failed with code: {}!",
                    static_cast<int>(rc));
        // TODO: Create the present semaphore as well
    }
}

void Hush::VulkanRenderer::DestroySwapChain()
{
    vkDestroySwapchainKHR(this->m_device, this->m_swapChain, nullptr);

    for (auto &imageView : this->m_swapchainImageViews)
    {
        vkDestroyImageView(this->m_device, imageView, nullptr);
    }
    this->m_swapchainImageViews.clear();
}

VkSubmitInfo2 Hush::VulkanRenderer::SubmitInfo(VkCommandBufferSubmitInfo *cmd,
                                               VkSemaphoreSubmitInfo *signalSemaphoreInfo,
                                               VkSemaphoreSubmitInfo *waitSemaphoreInfo)
{
    VkSubmitInfo2 info = {};
    info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
    info.pNext = nullptr;

    info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
    info.pWaitSemaphoreInfos = waitSemaphoreInfo;

    info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
    info.pSignalSemaphoreInfos = signalSemaphoreInfo;

    info.commandBufferInfoCount = 1;
    info.pCommandBufferInfos = cmd;

    return info;
}
