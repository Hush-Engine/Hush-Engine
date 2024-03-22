#include "VulkanImGuiForwarder.hpp"
#include "../VulkanRenderer.hpp"
#include "../../log/Logger.hpp"
#include <volk.h>
#include <imgui.h>
#include <imgui_impl_vulkan.h>


void Hush::VulkanImGuiForwarder::SetupImGui(const IRenderer &renderer)
{
    // Validate type checks
    if (!IsCorrectRendererType(renderer))
    {
        Hush::LogError("Tried to setup Dear ImGui with a renderer that does not match the target graphics API (Vulkan)");
        return;
    }
	//Cast the renderer to a Vulkan renderer
    const auto &vulkanRenderer = dynamic_cast<const VulkanRenderer&>(renderer);
	//Setup the code here for ImGui

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    // IO forwarding
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
    ImGui_ImplVulkan_InitInfo initData = this->CreateInitData(vulkanRenderer);
    ImGui_ImplVulkan_Init(&initData, vulkanRenderer.GetVulkanRenderPass());
}

bool Hush::VulkanImGuiForwarder::IsCorrectRendererType(const IRenderer &renderer)
{
    return typeid(renderer) == typeid(VulkanRenderer);
}

ImGui_ImplVulkan_InitInfo Hush::VulkanImGuiForwarder::CreateInitData(
    const VulkanRenderer &vulkanRenderer) const noexcept
{
    ImGui_ImplVulkan_InitInfo initData = {};
    initData.Instance = vulkanRenderer.GetVulkanInstance();
    initData.PhysicalDevice = vulkanRenderer.GetVulkanPhysicalDevice();
    initData.Device = vulkanRenderer.GetVulkanDevice();
    //TODO? Queue for initialization: initData.Queue = _graphicsQueue;
    initData.Queue = vulkanRenderer.GetGraphicsQueue();
    initData.DescriptorPool = this->CreateImGuiPool(initData.Device);
    //?? Why 3, idk
    initData.MinImageCount = 3;
    initData.ImageCount = 3;
    initData.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
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
    poolInfo.pPoolSizes = poolSizes;

    VkDescriptorPool imguiPool = {};
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &imguiPool);
    return imguiPool;
}
