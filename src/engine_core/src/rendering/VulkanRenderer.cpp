/*! \file VulkanRenderer.cpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Vulkan implementation for rendering
*/

#include "VulkanRenderer.hpp"
#include "log/Logger.hpp"
#include "utils/Platform.hpp"

#include <SDL2/SDL_vulkan.h>
#include <VkBootstrap.h>

#if HUSH_PLATFORM_WIN
#define VK_USE_PLATFORM_WIN32_KHR
#elif HUSH_PLATFORM_LINUX
#endif
#define VOLK_IMPLEMENTATION
#include <volk.h>

Hush::VulkanRenderer::VulkanRenderer(void *windowContext)
    : Hush::IRenderer(windowContext), m_windowContext(windowContext)
{
    LogTrace("Initializing Vulkan");
    volkInitialize();

    // Simplify vulkan creation with VkBootstrap
    // TODO: we might use the vulkan API without another dependency in the future???
    vkb::InstanceBuilder builder{};

    vkb::Result<vkb::Instance> instanceResult =
        builder.set_app_name("Hush Engine")
            .request_validation_layers(true)
            // TODO: We might use a lower version for some platforms such as Android
            .require_api_version(1, 3, 0)
            .build();

    if (!instanceResult)
    {
        LogFormat(ELogLevel::Critical, "Cannot load instance: {}", instanceResult.error().message());
    }

    vkb::Instance vkbInstance = instanceResult.value();

    LogTrace("Got vulkan instance");

    this->m_vulkanInstance = vkbInstance.instance;
    this->m_debugMessenger = vkbInstance.debug_messenger;
    volkLoadInstance(this->m_vulkanInstance);

    if (SDL_Vulkan_CreateSurface(static_cast<SDL_Window *>(windowContext), this->m_vulkanInstance, &this->m_surface) ==
        SDL_FALSE)
    {
        LogFormat(ELogLevel::Error, "Cannot create vulkan surface, error: {}", SDL_GetError());
    }

    LogTrace("Initialized vulkan surface");

    VkPhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.dynamicRendering = VK_TRUE;
    vulkan13Features.synchronization2 = VK_TRUE;

    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.bufferDeviceAddress = VK_TRUE;
    vulkan12Features.descriptorIndexing = VK_TRUE;

    vkb::PhysicalDeviceSelector selector{vkbInstance};
    vkb::PhysicalDevice vkbPhysicalDevice = selector.set_minimum_version(1, 3)
                                                .set_required_features_13(vulkan13Features)
                                                .set_surface(m_surface)
                                                .select()
                                                .value();

    vkb::DeviceBuilder deviceBuilder(vkbPhysicalDevice);

    vkb::Device vkbDevice = deviceBuilder.build().value();

    this->m_device = vkbDevice.device;
    this->m_vulkanPhysicalDevice = vkbDevice.physical_device;

    volkLoadDevice(this->m_device);

    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(this->m_vulkanPhysicalDevice, &properties);

    LogFormat(ELogLevel::Info, "Device name: {}", properties.deviceName);
    LogFormat(ELogLevel::Info, "API version: {}", properties.apiVersion);
}

Hush::VulkanRenderer::VulkanRenderer(VulkanRenderer &&rhs) noexcept : IRenderer(nullptr), m_windowContext(rhs.m_windowContext), m_vulkanInstance(rhs.m_vulkanInstance), m_vulkanPhysicalDevice(rhs.m_vulkanPhysicalDevice), m_debugMessenger(rhs.m_debugMessenger), m_device(rhs.m_device), m_surface(rhs.m_surface), m_swapChain(rhs.m_swapChain), m_swapchainImageFormat(rhs.m_swapchainImageFormat), m_swapchainImages(std::move(rhs.m_swapchainImages)), m_swapchainImageViews(std::move(rhs.m_swapchainImageViews)), m_swapChainExtent(rhs.m_swapChainExtent)
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
    this->DestroySwapChain();

    vkDestroySurfaceKHR(this->m_vulkanInstance, this->m_surface, nullptr);
    vkDestroyDevice(this->m_device, nullptr);
    vkDestroyInstance(this->m_vulkanInstance, nullptr);
}

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

    this->m_swapChainExtent = vkbSwapChain.extent;
    this->m_swapChain = vkbSwapChain.swapchain;
    this->m_swapchainImages = vkbSwapChain.get_images().value();
    this->m_swapchainImageViews = vkbSwapChain.get_image_views().value();
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
