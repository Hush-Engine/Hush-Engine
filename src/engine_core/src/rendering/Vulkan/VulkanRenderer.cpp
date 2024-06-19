/*! \file VulkanRenderer.cpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Vulkan implementation for rendering
*/

#include "rendering/Shared/RenderObject.hpp"
#include <cstddef>
#include <vulkan/vulkan_core.h>
#define VMA_IMPLEMENTATION
#include "VulkanRenderer.hpp"
#include "log/Logger.hpp"
#include "rendering/WindowManager.hpp"
#include "utils/Platform.hpp"

#include "rendering/Vulkan/VkTypes.hpp"

#include <SDL2/SDL_vulkan.h>

#if HUSH_PLATFORM_WIN
#define VK_USE_PLATFORM_WIN32_KHR
#elif HUSH_PLATFORM_LINUX
#endif
#define VOLK_IMPLEMENTATION
#include "VkUtilsFactory.hpp"
#include "rendering/ImGui/VulkanImGuiForwarder.hpp"
#include "utils/Assertions.hpp"
#include "vk_mem_alloc.hpp"
#include <utils/typeutils/TypeUtils.hpp>
#include <volk.h>
#include "VulkanVertexBuffer.hpp"
#include "VkOperations.hpp"

PFN_vkVoidFunction Hush::VulkanRenderer::CustomVulkanFunctionLoader(const char *functionName, void *userData)
{
    auto *mainVulkanRenderer = dynamic_cast<VulkanRenderer *>(WindowManager::GetMainWindow()->GetInternalRenderer());
    VkInstance mainWindowInstance = mainVulkanRenderer->GetVulkanInstance();
    PFN_vkVoidFunction result = vkGetInstanceProcAddr(mainWindowInstance, functionName);
    (void)userData; // Ignore user data
    return result;
}

Hush::VulkanRenderer::VulkanRenderer(void *windowContext)
    : Hush::IRenderer(windowContext), m_windowContext(windowContext)
{
    LogTrace("Initializing Vulkan");

    VkResult rc = volkInitialize();
    HUSH_VK_ASSERT(rc, "Error initializing Vulkan renderer!");

    // Simplify vulkan creation with VkBootstrap
    // TODO: we might use the vulkan API without another dependency in the future???
    vkb::InstanceBuilder builder{};
    vkb::Result<vkb::Instance> instanceResult =
        builder.set_app_name("Hush Engine")
            .request_validation_layers(true)
            .enable_extension(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
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
    this->LoadDebugMessenger();
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
    //> Init_Swapchain
    // draw image size will match the window
    VkExtent3D drawImageExtent = {this->m_width, this->m_height, 1};

    // hardcoding the draw format to 32 bit float
    this->m_drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
    this->m_drawImage.imageExtent = drawImageExtent;

    VkImageUsageFlags drawImageUsages{};
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_STORAGE_BIT;
    drawImageUsages |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    VkImageCreateInfo rimgInfo =
        VkUtilsFactory::CreateImageCreateInfo(this->m_drawImage.imageFormat, drawImageUsages, drawImageExtent);

    // for the draw image, we want to allocate it from gpu local memory
    VmaAllocationCreateInfo rimgAllocInfo = {};
    rimgAllocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
    rimgAllocInfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    // allocate and create the image
    vmaCreateImage(this->m_allocator, &rimgInfo, &rimgAllocInfo, &this->m_drawImage.image,
                   &this->m_drawImage.allocation, nullptr);

    // build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo rViewInfo = VkUtilsFactory::CreateImageViewCreateInfo(
        this->m_drawImage.imageFormat, this->m_drawImage.image, VK_IMAGE_ASPECT_COLOR_BIT);

    HUSH_VK_ASSERT(vkCreateImageView(this->m_device, &rViewInfo, nullptr, &this->m_drawImage.imageView),
                   "Failed to create image view");

    //Create depth image
    this->m_depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
    this->m_depthImage.imageExtent = drawImageExtent;

    VkImageUsageFlags depthImageUsages = {};
    depthImageUsages |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    VkImageCreateInfo depthImageInfo =
        VkUtilsFactory::CreateImageCreateInfo(this->m_depthImage.imageFormat, depthImageUsages, drawImageExtent);

    // allocate and create the image
    vmaCreateImage(this->m_allocator, &depthImageInfo, &rimgAllocInfo, &this->m_depthImage.image, &this->m_depthImage.allocation, nullptr);

    // build a image-view for the draw image to use for rendering
    VkImageViewCreateInfo depthViewInfo = VkUtilsFactory::CreateImageViewCreateInfo(
        this->m_depthImage.imageFormat, this->m_depthImage.image, VK_IMAGE_ASPECT_DEPTH_BIT);

    HUSH_VK_ASSERT(vkCreateImageView(this->m_device, &depthViewInfo, nullptr, &this->m_depthImage.imageView), "Failed to create depth image view");

    // add to deletion queues
    this->m_mainDeletionQueue.PushFunction([=]() {
        vkDestroyImageView(m_device, m_drawImage.imageView, nullptr);
        vmaDestroyImage(m_allocator, m_drawImage.image, m_drawImage.allocation);

        //Now destroy the depth image
        vkDestroyImageView(m_device, m_depthImage.imageView, nullptr);
        vmaDestroyImage(m_allocator, m_depthImage.image, m_depthImage.allocation);
    });
    //< Init_Swapchain
}

void Hush::VulkanRenderer::InitializeCommands() noexcept
{
    VkCommandPoolCreateInfo commandPoolInfo = VkUtilsFactory::CreateCommandPoolInfo(
        this->m_graphicsQueueFamily, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VkResult rc{};

    // Initialize the immediate command structures
    rc = vkCreateCommandPool(this->m_device, &commandPoolInfo, nullptr, &this->m_immediateCommandPool);
    HUSH_VK_ASSERT(rc, "Creating immediate command pool failed!");

    // allocate the command buffer for immediate submits
    VkCommandBufferAllocateInfo cmdAllocInfo =
        VkUtilsFactory::CreateCommandBufferAllocateInfo(this->m_immediateCommandPool);
    rc = vkAllocateCommandBuffers(this->m_device, &cmdAllocInfo, &this->m_immediateCommandBuffer);
    HUSH_VK_ASSERT(rc, "Allocating immidiate command buffers failed!");

    this->m_mainDeletionQueue.PushFunction([=]() { vkDestroyCommandPool(m_device, m_immediateCommandPool, nullptr); });

    for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
    {
        // Get a REFERENCE to the current frame
        rc = vkCreateCommandPool(this->m_device, &commandPoolInfo, nullptr, &this->m_frames.at(i).commandPool);
        HUSH_VK_ASSERT(rc, "Creating command pool failed!");

        // allocate the default command buffer that we will use for rendering
        cmdAllocInfo = VkUtilsFactory::CreateCommandBufferAllocateInfo(this->m_frames.at(i).commandPool);
        rc = vkAllocateCommandBuffers(this->m_device, &cmdAllocInfo, &this->m_frames.at(i).mainCommandBuffer);
        HUSH_VK_ASSERT(rc, "Allocating command buffers failed!");
        this->m_mainDeletionQueue.PushFunction(
            [=]() { vkDestroyCommandPool(m_device, m_frames.at(i).commandPool, nullptr); });
    }
}

void Hush::VulkanRenderer::InitDescriptors() noexcept
{
    // create a descriptor pool
    std::vector<DescriptorAllocator::PoolSizeRatio> sizes = {
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
    };

    this->m_globalDescriptorAllocator.InitPool(this->m_device, 10, sizes);
    this->m_mainDeletionQueue.PushFunction(
        [&]() { vkDestroyDescriptorPool(m_device, m_globalDescriptorAllocator.pool, nullptr); });

    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        this->m_drawImageDescriptorLayout = builder.Build(this->m_device, VK_SHADER_STAGE_COMPUTE_BIT);
    }
    {
        DescriptorLayoutBuilder builder;
        builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
        this->m_gpuSceneDataDescriptorLayout =
            builder.Build(this->m_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    }

    this->m_mainDeletionQueue.PushFunction([&]() {
        vkDestroyDescriptorSetLayout(m_device, m_drawImageDescriptorLayout, nullptr);
        vkDestroyDescriptorSetLayout(m_device, m_gpuSceneDataDescriptorLayout, nullptr);
    });

    this->m_drawImageDescriptors = this->m_globalDescriptorAllocator.Allocate(this->m_device, m_drawImageDescriptorLayout);
    {
        DescriptorWriter writer;
        writer.WriteImage(0, this->m_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL,
                           VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
        writer.UpdateSet(this->m_device, this->m_drawImageDescriptors);
    }

    for (int i = 0; i < FRAME_OVERLAP; i++)
    {
        // create a descriptor pool
        std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frame_sizes = {
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
        };

        this->m_frames.at(i).frameDescriptors = DescriptorAllocatorGrowable{};
        this->m_frames.at(i).frameDescriptors.Init(this->m_device, 1000, frame_sizes);
        this->m_mainDeletionQueue.PushFunction([&, i]() { m_frames.at(i).frameDescriptors.DestroyPool(m_device); });
    }
}

void Hush::VulkanRenderer::InitImGui()
{
    this->m_uiForwarder = std::make_unique<VulkanImGuiForwarder>();
    this->m_uiForwarder->SetupImGui(this);
}

void Hush::VulkanRenderer::Draw()
{
    FrameData &currentFrame = this->GetCurrentFrame();
    // wait until the gpu has finished rendering the last frame. Timeout of 1 second
    const uint32_t fenceTargetCount = 1u;
    // Wait until the gpu has finished rendering the last frame. Timeout of 1 second
    VkResult rc =
        vkWaitForFences(this->m_device, fenceTargetCount, &currentFrame.renderFence, VK_TRUE, VK_OPERATION_TIMEOUT_NS);
    HUSH_VK_ASSERT(rc, "Fence wait failed!");

    currentFrame.deletionQueue.Flush();

    // Request an image from the swapchain
    uint32_t swapchainImageIndex = 0u;
    rc = vkAcquireNextImageKHR(this->m_device, this->m_swapChain, VK_OPERATION_TIMEOUT_NS,
                               currentFrame.swapchainSemaphore, nullptr, &swapchainImageIndex);
    HUSH_VK_ASSERT(rc, "Image request from the swapchain failed!");

    rc = vkResetFences(this->m_device, fenceTargetCount, &currentFrame.renderFence);
    HUSH_VK_ASSERT(rc, "Fence reset failed!");

    // Get the command buffer and reset it
    VkCommandBuffer cmd = currentFrame.mainCommandBuffer;
    // Reset the command buffer
    rc = vkResetCommandBuffer(cmd, 0u);
    HUSH_VK_ASSERT(rc, "Reset command buffer failed!");

    // begin the command buffer recording. We will use this command buffer exactly once, so we want to let vulkan know
    // that
    VkCommandBufferBeginInfo cmdBeginInfo =
        VkUtilsFactory::CreateCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    rc = vkBeginCommandBuffer(cmd, &cmdBeginInfo);
    HUSH_VK_ASSERT(rc, "Begin command buffer failed!");

    VkRenderingAttachmentInfo colorAttachment = VkUtilsFactory::CreateAttachmentInfoWithLayout(
        this->m_swapchainImageViews.at(swapchainImageIndex), nullptr, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
    VkRenderingInfo renderingInfo =
        VkUtilsFactory::CreateRenderingInfo(this->m_swapChainExtent, &colorAttachment, nullptr);
    // execute a copy from the draw image into the swapchain
    VkExtent2D drawExtent = {this->m_width, this->m_height};
    this->CopyImageToImage(cmd, this->m_drawImage.image, this->m_swapchainImages.at(swapchainImageIndex), drawExtent,
                           this->m_swapChainExtent);

    // set swapchain image layout to Attachment Optimal so we can draw it
    // this->TransitionImage(cmd, this->m_swapchainImages.at(swapchainImageIndex), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    // VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

    vkCmdBeginRendering(cmd, &renderingInfo);
    // draw imgui into the swapchain image
    auto *uiImpl = dynamic_cast<VulkanImGuiForwarder *>(this->m_uiForwarder.get());
    uiImpl->RenderFrame(cmd);
    // set swapchain image layout to Present so we can draw it
    vkCmdEndRendering(cmd);
    this->TransitionImage(cmd, this->m_swapchainImages.at(swapchainImageIndex),
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

    // finalize the command buffer (we can no longer add commands, but it can now be executed)
    HUSH_VK_ASSERT(vkEndCommandBuffer(cmd), "End command buffer failed!");
    //< imgui_draw

    // prepare the submission to the queue.
    // we want to wait on the _presentSemaphore, as that semaphore is signaled when the swapchain is ready
    // we will signal the _renderSemaphore, to signal that rendering has finished

    VkCommandBufferSubmitInfo cmdinfo = VkUtilsFactory::CreateCommandBufferSubmitInfo(cmd);

    VkSemaphoreSubmitInfo waitInfo = VkUtilsFactory::CreateSemaphoreSubmitInfo(
        VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT_KHR, currentFrame.swapchainSemaphore);

    VkSemaphoreSubmitInfo signalInfo =
        VkUtilsFactory::CreateSemaphoreSubmitInfo(VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT, currentFrame.renderSemaphore);

    VkSubmitInfo2 submit = VkUtilsFactory::SubmitInfo(&cmdinfo, &signalInfo, &waitInfo);

    // submit command buffer to the queue and execute it.
    //  _renderFence will now block until the graphic commands finish execution
    HUSH_VK_ASSERT(vkQueueSubmit2(this->m_graphicsQueue, 1, &submit, currentFrame.renderFence), "Queue submit failed!");

    // prepare present
    //  this will put the image we just rendered to into the visible window.
    //  we want to wait on the _renderSemaphore for that,
    //  as its necessary that drawing commands have finished before the image is displayed to the user
    VkPresentInfoKHR presentInfo = VkUtilsFactory::CreatePresentInfo();

    presentInfo.pSwapchains = &this->m_swapChain;
    presentInfo.swapchainCount = 1;

    presentInfo.pWaitSemaphores = &currentFrame.renderSemaphore;
    presentInfo.waitSemaphoreCount = 1;

    presentInfo.pImageIndices = &swapchainImageIndex;

    VkResult presentResult = vkQueuePresentKHR(this->m_graphicsQueue, &presentInfo);

    HUSH_VK_ASSERT(presentResult, "Presenting failed!");

    // increase the number of frames drawn
    this->m_frameNumber++;
}

void Hush::VulkanRenderer::NewUIFrame() const noexcept
{
    this->m_uiForwarder->NewFrame();
}

void Hush::VulkanRenderer::HandleEvent(const SDL_Event *event) noexcept
{
    this->m_uiForwarder->HandleEvent(event);
}

void Hush::VulkanRenderer::InitRendering()
{
    this->InitializeCommands();
    this->CreateSyncObjects();
    this->InitDescriptors();
    //InitPipelines(?
    this->InitRenderables();
}

void Hush::VulkanRenderer::Dispose()
{
    this->m_uiForwarder->Dispose();
    LogTrace("Disposed of ImGui resources");

    if (this->m_device != nullptr)
    {
        vkDeviceWaitIdle(this->m_device);
        //Clear scenes and everything on our deletion queues
        this->m_loadedScenes.clear();
        this->m_mainDeletionQueue.Flush();
        for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
        {
            this->m_frames.at(i).deletionQueue.Flush();
            // Delete any command pools
            vkDestroyCommandPool(this->m_device, this->m_frames.at(i).commandPool, nullptr);
            // Destroy the sync objects
            vkDestroyFence(this->m_device, this->m_frames.at(i).renderFence, nullptr);
            vkDestroySemaphore(this->m_device, this->m_frames.at(i).renderSemaphore, nullptr);
            vkDestroySemaphore(this->m_device, this->m_frames.at(i).swapchainSemaphore, nullptr);
        }
        this->DestroySwapChain();
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
    if (this->m_allocator != nullptr) 
    {
        vmaDestroyAllocator(this->m_allocator);   
    }
    LogTrace("Vulkan resources destroyed");
}

void Hush::VulkanRenderer::ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function) noexcept
{
    VkResult rc = vkResetFences(this->m_device, 1u, &this->m_immediateFence);
    HUSH_VK_ASSERT(rc, "Failed to reset immediate fence!");

    rc = vkResetCommandBuffer(this->m_immediateCommandBuffer, 0);
    HUSH_VK_ASSERT(rc, "Failed to reset immediate command buffer!");

    VkCommandBufferBeginInfo cmdBeginInfo =
        VkUtilsFactory::CreateCommandBufferBeginInfo(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
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

Hush::AllocatedImage Hush::VulkanRenderer::GetDrawImage() const noexcept
{
    return this->m_drawImage;
}

Hush::AllocatedImage Hush::VulkanRenderer::GetDepthImage() const noexcept
{
    return this->m_depthImage;
}

VkDescriptorSetLayout Hush::VulkanRenderer::GetGpuSceneDataDescriptorLayout() const noexcept
{
    return this->m_gpuSceneDataDescriptorLayout;
}

FrameData &Hush::VulkanRenderer::GetCurrentFrame() noexcept
{
    return this->m_frames.at(this->m_frameNumber % FRAME_OVERLAP);
}

FrameData &Hush::VulkanRenderer::GetLastFrame() noexcept
{
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

    // Initialize our allocator
    this->InitVmaAllocator();

    LogFormat(ELogLevel::Debug, "Device name: {}", properties.deviceName);
    LogFormat(ELogLevel::Debug, "API version: {}", properties.apiVersion);
}

VkFormat *Hush::VulkanRenderer::GetSwapchainImageFormat() noexcept
{
    return &this->m_swapchainImageFormat;
}

void Hush::VulkanRenderer::CreateSyncObjects()
{
    // Create our sync objects and see if we were succesful
    VkFenceCreateInfo fenceInfo = VkUtilsFactory::CreateFenceInfo(VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT);
    VkSemaphoreCreateInfo semaphoreInfo = VkUtilsFactory::CreateSemaphoreInfo();

    VkResult rc = vkCreateFence(this->m_device, &fenceInfo, nullptr, &this->m_immediateFence);
    HUSH_VK_ASSERT(rc, "Immediate fence creation failed!");

    this->m_mainDeletionQueue.PushFunction([=]() { vkDestroyFence(m_device, m_immediateFence, nullptr); });

    for (uint32_t i = 0; i < FRAME_OVERLAP; i++)
    {
        rc = vkCreateFence(this->m_device, &fenceInfo, nullptr, &this->m_frames.at(i).renderFence);
        HUSH_VK_ASSERT(rc, "Creating fence failed!");

        // Create the semaphores
        rc = vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_frames.at(i).swapchainSemaphore);
        HUSH_VK_ASSERT(rc, "Creating swapchain semaphore failed!");

        rc = vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_frames.at(i).renderSemaphore);
        HUSH_VK_ASSERT(rc, "Creating render semaphore failed!");

        this->m_mainDeletionQueue.PushFunction([=]() {
            vkDestroyFence(m_device, m_frames.at(i).renderFence, nullptr);
            vkDestroySemaphore(m_device, m_frames.at(i).swapchainSemaphore, nullptr);
            vkDestroySemaphore(m_device, m_frames.at(i).renderSemaphore, nullptr);
        });
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

void *Hush::VulkanRenderer::GetWindowContext() const noexcept
{
    return this->m_windowContext;
}

const RenderStats &Hush::VulkanRenderer::GetStats() const noexcept
{
    return this->m_renderStats;
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

void Hush::VulkanRenderer::LoadDebugMessenger()
{
    VkDebugUtilsMessengerCreateInfoEXT createInfo = VkUtilsFactory::CreateDebugMessengerInfo(LogDebugMessage);
    VkResult rc = vkCreateDebugUtilsMessengerEXT(this->m_vulkanInstance, &createInfo, nullptr, &this->m_debugMessenger);
    HUSH_VK_ASSERT(rc, "Failed to create debug utils messenger!");
}

uint32_t Hush::VulkanRenderer::LogDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                               VkDebugUtilsMessageTypeFlagsEXT messageTypes,
                                               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                               void *pUserData)
{
    (void)(pCallbackData->pMessage);
    (void)messageSeverity;
    (void)messageTypes;
    (void)pUserData;
    return 0;
}

void Hush::VulkanRenderer::InitVmaAllocator()
{
    // Fetch the Vulkan functions using the Vulkan loader
    auto fpGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(
        vkGetInstanceProcAddr(this->m_vulkanInstance, "vkGetInstanceProcAddr"));
    auto fpGetDeviceProcAddr =
        reinterpret_cast<PFN_vkGetDeviceProcAddr>(vkGetDeviceProcAddr(this->m_device, "vkGetDeviceProcAddr"));

    // Fill in the VmaVulkanFunctions struct
    VmaVulkanFunctions vulkanFunctions{};
    vulkanFunctions.vkGetInstanceProcAddr = fpGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = fpGetDeviceProcAddr;

    // initialize the memory allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = this->m_vulkanPhysicalDevice;
    allocatorInfo.device = this->m_device;
    allocatorInfo.instance = this->m_vulkanInstance;
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;
    vmaCreateAllocator(&allocatorInfo, &this->m_allocator);

    this->m_mainDeletionQueue.PushFunction([&]() { vmaDestroyAllocator(m_allocator); });
}

void Hush::VulkanRenderer::InitRenderables()
{
    //TODO: Make
    std::string structurePath = {R"(..\..\assets\structure.glb)"};
    auto structureFile = VkOperations::LoadGltf(this, structurePath);
    HUSH_ASSERT(structureFile.get() != nullptr, "GLTF structure failed to load");
    this->m_loadedScenes["structure"] = structureFile;
}

void Hush::VulkanRenderer::TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout,
                                           VkImageLayout newLayout)
{
    VkImageMemoryBarrier2 imageBarrier{};
    imageBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    imageBarrier.pNext = nullptr;

    imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
    imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
    imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

    imageBarrier.oldLayout = currentLayout;
    imageBarrier.newLayout = newLayout;

    VkImageAspectFlags aspectMask =
        (newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    imageBarrier.subresourceRange = VkUtilsFactory::ImageSubResourceRange(aspectMask);
    imageBarrier.image = image;

    VkDependencyInfo depInfo{};
    depInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    depInfo.pNext = nullptr;

    depInfo.imageMemoryBarrierCount = 1;
    depInfo.pImageMemoryBarriers = &imageBarrier;

    vkCmdPipelineBarrier2(cmd, &depInfo);
}

void Hush::VulkanRenderer::CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination,
                                            VkExtent2D srcSize, VkExtent2D dstSize)
{
    VkImageBlit2 blitRegion{};
    blitRegion.sType = VK_STRUCTURE_TYPE_IMAGE_BLIT_2;
    blitRegion.pNext = nullptr;

    blitRegion.srcOffsets[1].x = static_cast<int32_t>(srcSize.width);
    blitRegion.srcOffsets[1].y = static_cast<int32_t>(srcSize.height);
    blitRegion.srcOffsets[1].z = 1;

    blitRegion.dstOffsets[1].x = static_cast<int32_t>(dstSize.width);
    blitRegion.dstOffsets[1].y = static_cast<int32_t>(dstSize.height);
    blitRegion.dstOffsets[1].z = 1;

    blitRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.srcSubresource.baseArrayLayer = 0;
    blitRegion.srcSubresource.layerCount = 1;
    blitRegion.srcSubresource.mipLevel = 0;

    blitRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blitRegion.dstSubresource.baseArrayLayer = 0;
    blitRegion.dstSubresource.layerCount = 1;
    blitRegion.dstSubresource.mipLevel = 0;

    VkBlitImageInfo2 blitInfo{};

    blitInfo.sType = VK_STRUCTURE_TYPE_BLIT_IMAGE_INFO_2;
    blitInfo.pNext = nullptr;
    blitInfo.dstImage = destination;
    blitInfo.dstImageLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    blitInfo.srcImage = source;
    blitInfo.srcImageLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    blitInfo.filter = VK_FILTER_LINEAR;
    blitInfo.regionCount = 1;
    blitInfo.pRegions = &blitRegion;

    vkCmdBlitImage2(cmd, &blitInfo);
}

void Hush::VulkanRenderer::DrawGeometry(VkCommandBuffer cmd)
{
    std::vector<uint32_t> opaqueDraws;
    opaqueDraws.reserve(this->m_drawCommands.opaqueSurfaces.size());
    for (size_t i = 0; i < this->m_drawCommands.opaqueSurfaces.size(); i++)
    {
        RenderObject& surface = this->m_drawCommands.opaqueSurfaces.at(i);
        if (surface.IsVisible(this->m_sceneData.viewProjection)) {
            opaqueDraws.push_back(i);
        }
    }

    // sort the opaque surfaces by material and mesh
    std::sort(opaqueDraws.begin(), opaqueDraws.end(), [&](const auto &iA, const auto &iB) {
        const RenderObject &a = this->m_drawCommands.opaqueSurfaces.at(iA);
        const RenderObject &b = this->m_drawCommands.opaqueSurfaces.at(iB);
        if (a.material == b.material)
        {
            return a.indexBuffer < b.indexBuffer;
        }
        return a.material < b.material;
    });

    // allocate a new uniform buffer for the scene data
    constexpr uint32_t gpuBufferSize = sizeof(GPUSceneData);
    VulkanVertexBuffer gpuSceneDataBuffer(gpuBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VMA_MEMORY_USAGE_CPU_TO_GPU, this->m_allocator);

    // add it to the deletion queue of this frame so it gets deleted once its been used
    FrameData &currentFrame = this->GetCurrentFrame();
    currentFrame.deletionQueue.PushFunction(
        [=]() { vmaDestroyBuffer(m_allocator, gpuSceneDataBuffer.GetBuffer(), gpuSceneDataBuffer.GetAllocation()); });

    // write the buffer
    auto *sceneUniformData = static_cast<GPUSceneData *>(gpuSceneDataBuffer.GetAllocation()->GetMappedData());
    *sceneUniformData = this->m_sceneData;

    // create a descriptor set that binds that buffer and update it
    VkDescriptorSet globalDescriptor =
        currentFrame.frameDescriptors.Allocate(this->m_device, this->m_gpuSceneDataDescriptorLayout);

    DescriptorWriter writer;
    writer.WriteBuffer(0, gpuSceneDataBuffer.GetBuffer(), gpuBufferSize, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    writer.UpdateSet(this->m_device, globalDescriptor);

    MaterialPipeline *lastPipeline = nullptr;
    MaterialInstance *lastMaterial = nullptr;
    VkBuffer lastIndexBuffer = VK_NULL_HANDLE;

    auto draw = [&](const RenderObject &r) {
        if (r.material != lastMaterial)
        {
            lastMaterial = r.material;
            if (r.material->pipeline != lastPipeline)
            {

                lastPipeline = r.material->pipeline;
                vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->pipeline);
                vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 0, 1,
                                        &globalDescriptor, 0, nullptr);

                VkViewport viewport = {};
                viewport.x = 0;
                viewport.y = 0;
                viewport.width = static_cast<float>(m_width);
                viewport.height = static_cast<float>(m_height);
                viewport.minDepth = 0.f;
                viewport.maxDepth = 1.f;

                vkCmdSetViewport(cmd, 0, 1, &viewport);

                VkRect2D scissor = {};
                scissor.offset.x = 0;
                scissor.offset.y = 0;
                scissor.extent.width = m_width;
                scissor.extent.height = m_height;

                vkCmdSetScissor(cmd, 0, 1, &scissor);
            }

            vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, r.material->pipeline->layout, 1, 1,
                                    &r.material->materialSet, 0, nullptr);
        }
        if (r.indexBuffer != lastIndexBuffer)
        {
            lastIndexBuffer = r.indexBuffer;
            vkCmdBindIndexBuffer(cmd, r.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        }
        // calculate final mesh matrix
        GPUDrawPushConstants pushConstants = {};
        pushConstants.worldMatrix = r.transform;
        pushConstants.vertexBuffer = r.vertexBufferAddress;

        vkCmdPushConstants(cmd, r.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                           sizeof(GPUDrawPushConstants), &pushConstants);

        m_renderStats.drawCalls++;
        m_renderStats.triangles += r.indexCount / 3;
        vkCmdDrawIndexed(cmd, r.indexCount, 1, r.firstIndex, 0, 0);
    };

    //Preparing to draw the geometry frame, set the stats count to 0
    this->m_renderStats.drawCalls = 0;
    this->m_renderStats.triangles = 0;

    for (uint32_t &r : opaqueDraws)
    {
        draw(this->m_drawCommands.opaqueSurfaces.at(r));
    }

    for (RenderObject &r : this->m_drawCommands.transparentSurfaces)
    {
        draw(r);
    }

    // we delete the draw commands now that we processed them
    this->m_drawCommands.opaqueSurfaces.clear();
    this->m_drawCommands.transparentSurfaces.clear();
}
