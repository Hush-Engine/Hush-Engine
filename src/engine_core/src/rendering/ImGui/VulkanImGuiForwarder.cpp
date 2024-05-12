#include "VulkanImGuiForwarder.hpp"
#include "log/Logger.hpp"
#include "rendering/Vulkan/VulkanRenderer.hpp"
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl2.h>
#include <imgui/backends/imgui_impl_vulkan.h>
#include <volk.h>
#include <rendering/WindowManager.hpp>
#include <rendering/Vulkan/VkUtilsFactory.hpp>

void Hush::VulkanImGuiForwarder::SetupImGui(IRenderer* renderer)
{
    // Cast the renderer to a Vulkan renderer
    auto* vulkanRenderer = dynamic_cast<VulkanRenderer*>(renderer);
    // Setup the code here for ImGui

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // IO forwarding
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    ImGui_ImplVulkan_InitInfo initData = this->CreateInitData(vulkanRenderer);
    auto *sdlWindow = static_cast<SDL_Window *>(vulkanRenderer->GetWindowContext());
    //Load vulkan functions
    HUSH_ASSERT(ImGui_ImplVulkan_LoadFunctions(VulkanRenderer::CustomVulkanFunctionLoader), "Loading vulkan functions to imgui failed");

    HUSH_ASSERT(ImGui_ImplSDL2_InitForVulkan(sdlWindow), "ImGui SDL2 init failed with error: {}!");

    //Get the rendering functions
    HUSH_ASSERT(ImGui_ImplVulkan_Init(&initData), "ImGui Vulkan init failed");
}

void Hush::VulkanImGuiForwarder::RenderFrame(VkCommandBuffer cmd)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
}

ImGui_ImplVulkan_InitInfo Hush::VulkanImGuiForwarder::CreateInitData(
    VulkanRenderer* vulkanRenderer) const noexcept
{
    ImGui_ImplVulkan_InitInfo initData = {};
    initData.Instance = vulkanRenderer->GetVulkanInstance();
    initData.PhysicalDevice = vulkanRenderer->GetVulkanPhysicalDevice();
    initData.Device = vulkanRenderer->GetVulkanDevice();
    initData.Queue = vulkanRenderer->GetGraphicsQueue();
    initData.DescriptorPool = this->CreateImGuiPool(initData.Device);
    //?? Why 3, idk
    initData.MinImageCount = 3;
    initData.ImageCount = 3;
    initData.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    initData.UseDynamicRendering = true;

    // dynamic rendering parameters for imgui to use
    initData.PipelineRenderingCreateInfo = {};
    initData.PipelineRenderingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    initData.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    initData.PipelineRenderingCreateInfo.pColorAttachmentFormats = vulkanRenderer->GetSwapchainImageFormat();

    return initData;
}

VkDescriptorPool Hush::VulkanImGuiForwarder::CreateImGuiPool(VkDevice device) const noexcept
{
    VkDescriptorPoolSize poolSizes[] = {{VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
                                        {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
                                        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
                                        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    poolInfo.maxSets = 1000;
    poolInfo.poolSizeCount = static_cast<uint32_t>(std::size(poolSizes));
    poolInfo.pPoolSizes = static_cast<VkDescriptorPoolSize*>(poolSizes);

    VkDescriptorPool imguiPool = {};
    VkResult rc = vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiPool);
    HUSH_VK_ASSERT(rc, "Failed to create ImGui descriptor pool!");

    return imguiPool;
}
