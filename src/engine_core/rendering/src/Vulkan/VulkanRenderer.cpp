/*! \file VulkanRenderer.cpp
	\author Alan Ramirez Herrera
	\date 2024-03-03
	\brief Vulkan implementation for rendering
*/

#include "Shared/GpuAllocatedBuffer.hpp"
#include <memory>
#define VMA_IMPLEMENTATION
#define VK_NO_PROTOTYPES
#include "VulkanRenderer.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

#include <SDL2/SDL_vulkan.h>

#if HUSH_PLATFORM_WIN
#define VK_USE_PLATFORM_WIN32_KHR
#elif HUSH_PLATFORM_LINUX
#endif
#define VOLK_IMPLEMENTATION

#include "Assertions.hpp"
#include "ImGui/VulkanImGuiForwarder.hpp"
#include "VkUtilsFactory.hpp"
#include "VulkanPipelineBuilder.hpp"
#include "vk_mem_alloc.hpp"
#include <typeutils/TypeUtils.hpp>
#include <volk.h>
#include <vulkan/vulkan_core.h>
#include "Shared/GPUMeshBuffers.hpp"
#include "VulkanLoader.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include "VulkanFullScreenPass.hpp"
#include <Shared/ShaderMaterial.hpp>
#include "Vector3Math.hpp"
#include "Shared/DirectionalLight.hpp"
#include <glm/gtx/string_cast.hpp>
#include "Renderer.hpp"
#include "Shared/GpuAllocatedImage.hpp"
#include "Shared/MaterialOptions.hpp"
#include "Shared/Types/Color.hpp"
#include <cstdint>
#include "Vulkan/VkTypes.hpp"
#include <magic_enum/magic_enum.hpp>
#include "../../core/src/Components/WorldTransform.hpp"
#include "../../core/src/Scene.hpp"

PFN_vkVoidFunction Hush::VulkanRenderer::CustomVulkanFunctionLoader(const char *functionName, void *userData)
{
	PFN_vkVoidFunction result = vkGetInstanceProcAddr(volkGetLoadedInstance(), functionName);
	(void)userData; // Ignore user data
	return result;
}

Hush::VulkanRenderer::VulkanRenderer(void *windowContext)
	: Hush::IRenderer(windowContext),
	  m_windowContext(windowContext),
	  m_globalDescriptorAllocator()
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
			//.enable_extension(VK_EXT_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)
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
	: IRenderer(nullptr),
	  m_windowContext(rhs.m_windowContext),
	  m_vulkanInstance(rhs.m_vulkanInstance),
	  m_vulkanPhysicalDevice(rhs.m_vulkanPhysicalDevice),
	  m_debugMessenger(rhs.m_debugMessenger),
	  m_device(rhs.m_device),
	  m_surface(rhs.m_surface),
	  m_swapchain(rhs.m_swapchain)
{
	rhs.m_vulkanInstance = nullptr;
	rhs.m_vulkanPhysicalDevice = nullptr;
	rhs.m_debugMessenger = nullptr;
	rhs.m_device = nullptr;
	rhs.m_surface = nullptr;
	rhs.m_swapchain = {};
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
		this->m_swapchain = rhs.m_swapchain;

		rhs.m_vulkanInstance = nullptr;
		rhs.m_vulkanPhysicalDevice = nullptr;
		rhs.m_debugMessenger = nullptr;
		rhs.m_device = nullptr;
		rhs.m_surface = nullptr;
		rhs.m_swapchain = {};
	}

	return *this;
}

Hush::VulkanRenderer::~VulkanRenderer()
{
	this->Dispose();
}

void Hush::VulkanRenderer::SetActiveScene(Scene *scene)
{
	this->m_activeScene = scene;
}

// Called on resize and window init
void Hush::VulkanRenderer::CreateSwapChain(uint32_t width, uint32_t height)
{
	this->m_width = width;
	this->m_height = height;
	this->m_swapchain.Recreate(width, height, this);
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

	this->AddToDeletionQueue([=, this]() { vkDestroyCommandPool(m_device, m_immediateCommandPool, nullptr); });

	for (int32_t i = 0; i < FRAME_OVERLAP; i++)
	{
		// Get a REFERENCE to the current frame
		rc = vkCreateCommandPool(this->m_device, &commandPoolInfo, nullptr, &this->m_frames.at(i).commandPool);
		HUSH_VK_ASSERT(rc, "Creating command pool failed!");

		// allocate the default command buffer that we will use for rendering
		cmdAllocInfo = VkUtilsFactory::CreateCommandBufferAllocateInfo(this->m_frames.at(i).commandPool);
		rc = vkAllocateCommandBuffers(this->m_device, &cmdAllocInfo, &this->m_frames.at(i).mainCommandBuffer);
		HUSH_VK_ASSERT(rc, "Allocating command buffers failed!");
		this->AddToDeletionQueue([=, this]() { vkDestroyCommandPool(m_device, m_frames.at(i).commandPool, nullptr); });
	}
}

void Hush::VulkanRenderer::InitImGui()
{
	this->m_uiForwarder = std::make_unique<VulkanImGuiForwarder>();
	this->m_uiForwarder->SetupImGui(this);
}

void Hush::VulkanRenderer::PushMesh(const glm::mat4 &globalTransform, std::shared_ptr<Mesh> mesh)
{
	// TODO: Make this take an entity or something like that so we have access to its transform
	// this->m_loadedNodes.emplace(std::make_shared<VulkanMeshNode>(mesh), mesh->GetName());
	(void)globalTransform;
	(void)mesh;
}

void Hush::VulkanRenderer::DestroyMesh(const std::string_view &name)
{
	(void)name;
}

void Hush::VulkanRenderer::Draw(float delta)
{
	if (this->m_resizeRequested)
	{
		this->ResizeSwapchain();
		return;
	}

	this->UpdateSceneObjects(delta);

	// Prepare and flush the render command
	FrameData &currentFrame = this->GetCurrentFrame();
	uint32_t swapchainImageIndex = 0u;
	VkCommandBuffer cmd = this->PrepareCommandBuffer(currentFrame, &swapchainImageIndex);

	if (cmd == nullptr)
	{
		return;
	}

	VkImage currentImage = this->m_swapchain.GetImages().at(swapchainImageIndex);

	this->TransitionImage(cmd, this->m_drawImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL);
	this->DrawBackground(cmd);
	// Transition
	this->TransitionImage(cmd, this->m_drawImage.image, VK_IMAGE_LAYOUT_GENERAL,
						  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	this->TransitionImage(cmd, this->m_depthImage.image, VK_IMAGE_LAYOUT_UNDEFINED,
						  VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);
	// Geometry
	this->DrawGeometry(cmd);

	// transtion the draw image and the swapchain image into their correct transfer layouts
	this->TransitionImage(cmd, this->m_drawImage.image, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
						  VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
	this->TransitionImage(cmd, currentImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	this->CopyImageToImage(cmd, this->m_drawImage.image, currentImage, {this->m_width, this->m_height},
						   this->m_swapchain.GetExtent());
	this->TransitionImage(cmd, currentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						  VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	// UI
	this->DrawUI(cmd, this->m_swapchain.GetImageViews()[swapchainImageIndex]);
	// set swapchain image layout to Present so we can draw it
	this->TransitionImage(cmd, currentImage, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	// finalize the command buffer (we can no longer add commands, but it can now be executed)
	HUSH_VK_ASSERT(vkEndCommandBuffer(cmd), "End command buffer failed!");

	this->m_swapchain.Present(cmd, &swapchainImageIndex, &this->m_resizeRequested);

	if (this->m_resizeRequested)
	{
		// Skip frame
		return;
	}

	// increase the number of frames drawn
	this->m_frameNumber++;
}

void Hush::VulkanRenderer::NewUIFrame() const noexcept
{
	this->m_uiForwarder->NewFrame();
}

void Hush::VulkanRenderer::EndUIFrame() const noexcept
{
	this->m_uiForwarder->EndFrame();
}

void Hush::VulkanRenderer::HandleEvent(const SDL_Event *event) noexcept
{
	this->m_uiForwarder->HandleEvent(event);
}

void DrawMesh(Hush::Mesh *mesh, const Hush::WorldTransform *transform, void *drawContext)
{

	// Interpret drawContext as: std::vector<VkRenderObject>* OpaqueSurfaces;
	HUSH_ASSERT(drawContext != nullptr, "Draw context should not be null for any render node");
	auto *drawCtxImpl = static_cast<Hush::DrawContext *>(drawContext);

	for (const Hush::GeoSurface &s : mesh->GetSurfaces())
	{
		Hush::VkRenderObject def{};
		def.indexCount = s.count;
		def.firstIndex = s.startIndex;
		def.indexBuffer = static_cast<VkBuffer>(mesh->GetMeshBuffers().indexBuffer.GetBuffer());
		// Replace with graphics API call
		def.material = s.material->GetInternalMaterial();

		def.transform = transform->GetTransformationMatrix();
		def.vertexBufferAddress = mesh->GetMeshBuffers().vertexBufferAddress;
		if (s.material->GetInternalMaterial()->passType == Hush::EMaterialPass::Transparent)
		{
			drawCtxImpl->transparentSurfaces.push_back(def);
		}
		else
		{
			drawCtxImpl->opaqueSurfaces.push_back(def);
		}
	}
}

void Hush::VulkanRenderer::UpdateSceneObjects(float delta)
{
	(void)delta;
	this->m_mainDrawContext.opaqueSurfaces.clear();
	this->m_mainDrawContext.transparentSurfaces.clear();
	// Test stuff just to show that it works... to be refactored into a more dynamic approach
	glm::mat4 topMatrix{1.0F};
	for (auto &nodeEntry : this->m_loadedMeshes)
	{
		DrawMesh(nodeEntry.second, nodeEntry.first, &this->m_mainDrawContext);
	}

	glm::mat4 scaleMat = glm::scale(glm::mat4(1.0F), Vector3Math::ONE);
	glm::mat4 viewMatrix = this->m_editorCamera.GetViewMatrix() * scaleMat;
	this->m_sceneData.view = viewMatrix;
	this->m_sceneData.proj = this->m_editorCamera.GetProjectionMatrix();

	// invert the Y direction on projection matrix so that we are more similar
	// to opengl and gltf axis
	this->m_sceneData.proj[1][1] *= -1;
	this->m_sceneData.viewproj = this->m_sceneData.proj * this->m_sceneData.view;

	// some default lighting parameters
	this->m_sceneData.ambientColor = glm::vec4(.1f);
	if (this->m_directionalLight != nullptr)
	{
		this->m_sceneData.sunlightColor = this->m_directionalLight->color.GetRGBA32F();
		glm::vec3 sunDir = this->m_sunTransform->Forward();
		this->m_sceneData.sunlightDirection = glm::vec4(sunDir, this->m_directionalLight->intensity);
	}
}

void Hush::VulkanRenderer::InitRendering()
{
	constexpr float initialFOV = 70.0F;
	this->m_editorCamera =
		EditorCamera(initialFOV, static_cast<float>(this->m_width), static_cast<float>(this->m_height), 0.1f, 4000.0f);

	this->CreateSyncObjects();

	this->InitializeCommands();

	this->InitDescriptors();

	this->InitPipelines();

	// Last two methods must be called after the pipelines and descriptors are initialized
	this->InitDefaultData();

	this->InitRenderables();
}

void Hush::VulkanRenderer::Dispose()
{
	this->m_uiForwarder->Dispose();
	LogTrace("Disposed of ImGui resources");
	if (this->m_device != nullptr)
	{
		vkDeviceWaitIdle(this->m_device);

		for (auto &mesh : this->m_loadedMeshes)
		{
			mesh.second->GetMeshBuffers().indexBuffer.Dispose(this->m_allocator);
			mesh.second->GetMeshBuffers().vertexBuffer.Dispose(this->m_allocator);
		}

		this->m_mainDeletionQueue.Flush();
		for (int i = 0; i < FRAME_OVERLAP; i++)
		{
			this->m_frames.at(i).deletionQueue.Flush();
			// Delete any command pools
			vkDestroyCommandPool(this->m_device, this->m_frames.at(i).commandPool, nullptr);
			// Destroy the sync objects
			vkDestroyFence(this->m_device, this->m_frames.at(i).renderFence, nullptr);
			vkDestroySemaphore(this->m_device, this->m_frames.at(i).renderSemaphore, nullptr);
			vkDestroySemaphore(this->m_device, this->m_frames.at(i).swapchainSemaphore, nullptr);
		}
		this->m_swapchain.Destroy();
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
	VkResult rc = vkResetFences(this->m_device, 1U, &this->m_immediateFence);
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

	rc = vkQueueSubmit2(this->m_graphicsQueue, 1U, &submit, this->m_immediateFence);
	HUSH_VK_ASSERT(rc, "Failed to submit graphics queue!");

	constexpr uint64_t fenceTimeout = 9999999999U;
	rc = vkWaitForFences(this->m_device, 1U, &this->m_immediateFence, VK_TRUE, fenceTimeout);
	HUSH_VK_ASSERT(rc, "Immediate fence timed out");
}

VkInstance Hush::VulkanRenderer::GetVulkanInstance() noexcept
{
	return this->m_vulkanInstance;
}

VkDevice Hush::VulkanRenderer::GetVulkanDevice() noexcept
{
	return this->m_device;
}

VkDescriptorSetLayout Hush::VulkanRenderer::GetGpuSceneDataDescriptorLayout() noexcept
{
	return this->m_gpuSceneDataDescriptorLayout;
}

const Hush::GpuAllocatedImage &Hush::VulkanRenderer::GetDrawImage() const noexcept
{
	return this->m_drawImage;
}

Hush::GpuAllocatedImage &Hush::VulkanRenderer::GetDrawImage() noexcept
{
	return this->m_drawImage;
}

const Hush::GpuAllocatedImage &Hush::VulkanRenderer::GetDepthImage() const noexcept
{
	return this->m_depthImage;
}

Hush::GpuAllocatedImage &Hush::VulkanRenderer::GetDepthImage() noexcept
{
	return this->m_depthImage;
}

VkPhysicalDevice Hush::VulkanRenderer::GetVulkanPhysicalDevice() const noexcept
{
	return this->m_vulkanPhysicalDevice;
}

VkQueue Hush::VulkanRenderer::GetGraphicsQueue() const noexcept
{
	return this->m_graphicsQueue;
}

Hush::FrameData &Hush::VulkanRenderer::GetCurrentFrame() noexcept
{
	return this->m_frames.at(this->m_frameNumber % FRAME_OVERLAP);
}

Hush::FrameData &Hush::VulkanRenderer::GetLastFrame() noexcept
{
	return this->m_frames.at((this->m_frameNumber - 1) % FRAME_OVERLAP);
}

VkSampler Hush::VulkanRenderer::GetDefaultSamplerLinear() noexcept
{
	return this->m_defaultSamplerLinear;
}

VkSampler Hush::VulkanRenderer::GetDefaultSamplerNearest() noexcept
{
	return this->m_defaultSamplerNearest;
}

Hush::GLTFMetallicRoughness &Hush::VulkanRenderer::GetMetalRoughMaterial() noexcept
{
	return this->m_metalRoughMaterial;
}

Hush::DescriptorAllocatorGrowable &Hush::VulkanRenderer::GlobalDescriptorAllocator() noexcept
{
	return this->m_globalDescriptorAllocator;
}

VmaAllocator Hush::VulkanRenderer::GetVmaAllocator() noexcept
{
	return this->m_allocator;
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
												.prefer_gpu_device_type(vkb::PreferredDeviceType::discrete)
												.set_required_features_13(vulkan13Features)
												.set_required_features_12(vulkan12Features)
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

void Hush::VulkanRenderer::CreateSyncObjects()
{
	// Create our sync objects and see if we were succesful
	VkFenceCreateInfo fenceInfo = VkUtilsFactory::CreateFenceInfo(VkFenceCreateFlagBits::VK_FENCE_CREATE_SIGNALED_BIT);
	VkSemaphoreCreateInfo semaphoreInfo = VkUtilsFactory::CreateSemaphoreInfo();

	VkResult rc = vkCreateFence(this->m_device, &fenceInfo, nullptr, &this->m_immediateFence);
	HUSH_VK_ASSERT(rc, "Immediate fence creation failed!");

	this->AddToDeletionQueue([=, this]() { vkDestroyFence(m_device, m_immediateFence, nullptr); });

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		rc = vkCreateFence(this->m_device, &fenceInfo, nullptr, &this->m_frames.at(i).renderFence);
		HUSH_VK_ASSERT(rc, "Creating fence failed!");

		// Create the semaphores
		rc = vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_frames.at(i).swapchainSemaphore);
		HUSH_VK_ASSERT(rc, "Creating swapchain semaphore failed!");

		rc = vkCreateSemaphore(this->m_device, &semaphoreInfo, nullptr, &this->m_frames.at(i).renderSemaphore);
		HUSH_VK_ASSERT(rc, "Creating render semaphore failed!");

		this->AddToDeletionQueue([=, this]() {
			vkDestroyFence(m_device, m_frames.at(i).renderFence, nullptr);
			vkDestroySemaphore(m_device, m_frames.at(i).swapchainSemaphore, nullptr);
			vkDestroySemaphore(m_device, m_frames.at(i).renderSemaphore, nullptr);
		});
	}
}

void *Hush::VulkanRenderer::GetWindowContext() const noexcept
{
	return this->m_windowContext;
}

void Hush::VulkanRenderer::SetDirectionalLight(DirectionalLight *light) noexcept
{
	(void)light;
	Query<DirectionalLight, WorldTransform> queryRes =
		this->m_activeScene->CreateQuery<DirectionalLight, WorldTransform>();
	queryRes.Each([this](DirectionalLight &lightComponent, WorldTransform &transformComponent) {
		this->m_directionalLight = &lightComponent;
		this->m_sunTransform = &transformComponent;
	});
}

const Hush::EditorCamera &Hush::VulkanRenderer::GetEditorCamera() const noexcept {
	return this->m_editorCamera;
}

Hush::EditorCamera* Hush::VulkanRenderer::GetEditorCamera() noexcept {
	return &this->m_editorCamera;
}

Hush::VulkanSwapchain &Hush::VulkanRenderer::GetSwapchain()
{
	return this->m_swapchain;
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
	Hush::LogFormat(ELogLevel::Critical, "Error from Vulkan: {}", pCallbackData->pMessage);
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

	this->AddToDeletionQueue([&]() { vmaDestroyAllocator(m_allocator); });
}

void Hush::VulkanRenderer::InitRenderables()
{
	std::string structurePath = R"(C:\Users\nefes\Personal\Hush-Engine\res\Lantern.glb)";
	// Create an example entity with a Mesh component here
	// std::string structurePath = R"(C:\Users\nefes\Personal\Hush-Engine\res\Duck.glb)";
	HUSH_ASSERT(this->m_activeScene != nullptr, "No scene has been set, please call SetActiveScene before rendering");
	std::vector<Entity> nodeVector = VulkanLoader::LoadGltfMeshes(this, structurePath, this->m_activeScene).value();
	for (Entity &node : nodeVector)
	{
		WorldTransform *xform = node.GetComponent<WorldTransform>();
		Mesh *mesh = node.GetComponent<Mesh>();
		std::pair<WorldTransform *, Mesh *> entry(xform, mesh);
		this->m_loadedMeshes.emplace_back(entry);
	}
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

	blitRegion.srcOffsets[1].x = srcSize.width;
	blitRegion.srcOffsets[1].y = srcSize.height;
	blitRegion.srcOffsets[1].z = 1;

	blitRegion.dstOffsets[1].x = dstSize.width;
	blitRegion.dstOffsets[1].y = dstSize.height;
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

void Hush::VulkanRenderer::InitDescriptors() noexcept
{
	// create a descriptor pool that will hold 10 sets with 1 image each
	std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1}};

	this->m_globalDescriptorAllocator.Init(volkGetLoadedDevice(), 10, sizes);

	// make the descriptor set layout for our compute draw
	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		this->m_gpuSceneDataDescriptorLayout =
			builder.Build(this->m_device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	}

	{
		DescriptorLayoutBuilder builder;
		builder.AddBinding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
		this->m_drawImageDescriptorLayout = builder.Build(this->m_device, VK_SHADER_STAGE_COMPUTE_BIT);
	}

	// allocate a descriptor set for our draw images
	this->m_drawImageDescriptors =
		this->m_globalDescriptorAllocator.Allocate(this->m_device, this->m_drawImageDescriptorLayout);

	DescriptorWriter writer;
	writer.WriteImage(0, this->m_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_GENERAL,
					  VK_DESCRIPTOR_TYPE_STORAGE_IMAGE);
	writer.UpdateSet(this->m_device, this->m_drawImageDescriptors);

	for (int i = 0; i < FRAME_OVERLAP; i++)
	{
		// create a descriptor pool
		std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> frameSizes = {
			{VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 3},
			{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 3},
			{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
			{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4},
		};

		this->m_frames[i].frameDescriptors = DescriptorAllocatorGrowable{};
		this->m_frames[i].frameDescriptors.Init(this->m_device, 1000, frameSizes);

		this->AddToDeletionQueue([&, i]() { m_frames[i].frameDescriptors.DestroyPool(m_device); });
	}

	// make sure both the descriptor allocator and the new layout get cleaned up properly
	this->AddToDeletionQueue([&]() {
		m_globalDescriptorAllocator.DestroyPool(m_device);
		vkDestroyDescriptorSetLayout(m_device, m_drawImageDescriptorLayout, nullptr);
	});

	DescriptorLayoutBuilder builder;
	builder.AddBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	this->m_singleImageDescriptorLayout = builder.Build(this->m_device, VK_SHADER_STAGE_FRAGMENT_BIT);
}

void Hush::VulkanRenderer::InitPipelines() noexcept
{
	this->InitBackgroundPipelines();
	this->InitMeshPipeline();

	// Just as a test, let's bind some shaders!
	std::filesystem::path frag(R"(C:\Users\nefes\Personal\Hush-Engine\res\grid.frag.spv)");
	std::filesystem::path vert(R"(C:\Users\nefes\Personal\Hush-Engine\res\grid.vert.spv)");

	auto gridMaterial = std::make_shared<ShaderMaterial>();
	gridMaterial->SetAlphaBlendMode(EAlphaBlendMode::OneMinusSrcAlpha);
	ShaderMaterial::EError err = gridMaterial->LoadShaders(this, frag, vert);
	this->m_gridEffect = VulkanFullScreenPass(this, gridMaterial);
	gridMaterial->GenerateMaterialInstance(&this->m_globalDescriptorAllocator);

	HUSH_ASSERT(err == ShaderMaterial::EError::None, "Failed to load shader material: {}", magic_enum::enum_name(err));
}

void Hush::VulkanRenderer::InitBackgroundPipelines() noexcept
{
	// First, define the push constant range
	VkPushConstantRange pushConstant{};
	pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	pushConstant.offset = 0;
	pushConstant.size = sizeof(ComputePushConstants);

	// layout code
	VkShaderModule computeDrawShader = nullptr;
	constexpr std::string_view shaderPath = R"(C:\Users\nefes\Personal\Hush-Engine\res\gradient_color.comp.spv)";
	if (!VulkanHelper::LoadShaderModule(shaderPath, this->m_device, &computeDrawShader))
	{
		LogError("Error when building the compute shader");
		return;
	}

	VkPipelineLayoutCreateInfo computeLayout{};
	computeLayout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	computeLayout.pNext = nullptr;
	computeLayout.pSetLayouts = &this->m_drawImageDescriptorLayout;
	computeLayout.setLayoutCount = 1;
	computeLayout.pushConstantRangeCount = 1;
	computeLayout.pPushConstantRanges = &pushConstant;

	VkResult res = vkCreatePipelineLayout(this->m_device, &computeLayout, nullptr, &this->m_gradientPipelineLayout);
	HUSH_VK_ASSERT(res, "Creating compute pipelines failed!");

	VkPipelineShaderStageCreateInfo stageinfo{};
	stageinfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageinfo.pNext = nullptr;
	stageinfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageinfo.module = computeDrawShader;
	stageinfo.pName = "main";

	VkComputePipelineCreateInfo computePipelineCreateInfo{};
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = nullptr;
	computePipelineCreateInfo.layout = this->m_gradientPipelineLayout;
	computePipelineCreateInfo.stage = stageinfo;
	res = vkCreateComputePipelines(this->m_device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr,
								   &this->m_gradientPipeline);
	HUSH_VK_ASSERT(res, "Creating compute pipelines failed!");

	// destroy structures properly
	vkDestroyShaderModule(this->m_device, computeDrawShader, nullptr);
	this->AddToDeletionQueue([=, this]() {
		vkDestroyPipelineLayout(this->m_device, this->m_gradientPipelineLayout, nullptr);
		vkDestroyPipeline(this->m_device, this->m_gradientPipeline, nullptr);
	});
}

void Hush::VulkanRenderer::InitMeshPipeline() noexcept
{
	constexpr std::string_view fragmentShaderPath = R"(C:\Users\nefes\Personal\Hush-Engine\res\tex_image.frag.spv)";
	constexpr std::string_view vertexShaderPath =
		R"(C:\Users\nefes\Personal\Hush-Engine\res\colored_triangle_mesh.vert.spv)";

	VkShaderModule triangleFragShader = nullptr;
	if (!VulkanHelper::LoadShaderModule(fragmentShaderPath, this->m_device, &triangleFragShader))
	{
		LogError("Error when building the triangle fragment shader module");
	}

	VkShaderModule triangleVertexShader = nullptr;
	if (!VulkanHelper::LoadShaderModule(vertexShaderPath, this->m_device, &triangleVertexShader))
	{
		LogError("Error when building the triangle vertex shader module");
	}

	VkPushConstantRange bufferRange{};
	bufferRange.offset = 0;
	bufferRange.size = sizeof(GPUDrawPushConstants);
	bufferRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// build the pipeline layout that controls the inputs/outputs of the shader
	// we are not using descriptor sets or other systems yet, so no need to use anything other than empty default
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = VkUtilsFactory::PipelineLayoutCreateInfo();
	pipelineLayoutInfo.pPushConstantRanges = &bufferRange;
	pipelineLayoutInfo.pushConstantRangeCount = 1;
	pipelineLayoutInfo.pSetLayouts = &this->m_singleImageDescriptorLayout;
	pipelineLayoutInfo.setLayoutCount = 1;
	VkResult rc = vkCreatePipelineLayout(this->m_device, &pipelineLayoutInfo, nullptr, &this->m_meshPipelineLayout);
	HUSH_VK_ASSERT(rc, "Failed to create triangle pipeline");

	VulkanPipelineBuilder pipelineBuilder(this->m_meshPipelineLayout);

	// connecting the vertex and pixel shaders to the pipeline
	pipelineBuilder.SetShaders(triangleVertexShader, triangleFragShader);
	// it will draw triangles
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	// filled triangles
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	// no backface culling
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	// no multisampling
	pipelineBuilder.SetMultiSamplingNone();

	pipelineBuilder.DisableBlending();

	pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	// connect the image format we will draw into, from draw image
	pipelineBuilder.SetColorAttachmentFormat(static_cast<VkFormat>(this->m_drawImage.imageFormat));
	pipelineBuilder.SetDepthFormat(static_cast<VkFormat>(this->m_depthImage.imageFormat));

	// finally build the pipeline
	this->m_meshPipeline = pipelineBuilder.Build(this->m_device);

	// clean structures
	vkDestroyShaderModule(this->m_device, triangleFragShader, nullptr);
	vkDestroyShaderModule(this->m_device, triangleVertexShader, nullptr);

	this->AddToDeletionQueue([=, this]() {
		vkDestroyPipelineLayout(m_device, m_meshPipelineLayout, nullptr);
		vkDestroyPipeline(m_device, m_meshPipeline, nullptr);
	});
}

void Hush::VulkanRenderer::InitDefaultData() noexcept
{
	std::array<Mesh::Vertex, 4> rectVertices;

	rectVertices[0].position = {0.5, -0.5, 0};
	rectVertices[1].position = {0.5, 0.5, 0};
	rectVertices[2].position = {-0.5, -0.5, 0};
	rectVertices[3].position = {-0.5, 0.5, 0};

	rectVertices[0].color = {1, 0, 1, 1};
	rectVertices[1].color = {0.5, 0.5, 0.5, 1};
	rectVertices[2].color = {1, 0, 0, 1};
	rectVertices[3].color = {0, 1, 0, 1};

	std::array<uint32_t, 6> rectIndices;

	rectIndices[0] = 0;
	rectIndices[1] = 1;
	rectIndices[2] = 2;

	rectIndices[3] = 2;
	rectIndices[4] = 1;
	rectIndices[5] = 3;

	m_rectangle = this->UploadMesh(std::vector<uint32_t>(rectIndices.begin(), rectIndices.end()),
								   std::vector<Mesh::Vertex>(rectVertices.begin(), rectVertices.end()));

	// delete the rectangle data on engine shutdown
	this->AddToDeletionQueue([&]() {
		m_rectangle.indexBuffer.Dispose(m_allocator);
		m_rectangle.vertexBuffer.Dispose(m_allocator);
	});

	// Default images
	this->m_defaultImageProvider.CreateDefaultImages(this);

	// checkerboard image
	uint32_t magenta = glm::packUnorm4x8(glm::vec4(1, 0, 1, 1));
	uint32_t black = Color::Black().ToColor32();
	std::array<uint32_t, 16 * 16> pixels; // for 16x16 checkerboard texture
	for (int x = 0; x < 16; x++)
	{
		for (int y = 0; y < 16; y++)
		{
			pixels[y * 16 + x] = ((x % 2) ^ (y % 2)) ? magenta : black;
		}
	}
	m_errorCheckerboardImage =
		CreateImage(pixels.data(), {16, 16, 1}, Color::EFormat::RGBA8Unorm, VK_IMAGE_USAGE_SAMPLED_BIT);

	VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};

	sampl.magFilter = VK_FILTER_NEAREST;
	sampl.minFilter = VK_FILTER_NEAREST;

	vkCreateSampler(m_device, &sampl, nullptr, &m_defaultSamplerNearest);

	sampl.magFilter = VK_FILTER_LINEAR;
	sampl.minFilter = VK_FILTER_LINEAR;
	vkCreateSampler(m_device, &sampl, nullptr, &m_defaultSamplerLinear);

	this->AddToDeletionQueue([&]() {
		vkDestroySampler(m_device, m_defaultSamplerNearest, nullptr);
		vkDestroySampler(m_device, m_defaultSamplerLinear, nullptr);

		DestroyImage(&m_errorCheckerboardImage);
	});
}

void Hush::VulkanRenderer::DrawGeometry(VkCommandBuffer cmd)
{
	////allocate a new uniform buffer for the scene data
	GpuAllocatedBuffer gpuSceneDataBuffer(sizeof(GPUSceneData), GpuAllocatedBuffer::EBufferUsage::UniformBuffer,
										  GpuAllocatedBuffer::EMemoryUsage::CpuToGpu, this->m_allocator);

	////write the buffer
	auto *sceneUniformData = static_cast<GPUSceneData *>(gpuSceneDataBuffer.GetMappedData());
	*sceneUniformData = this->m_sceneData;

	// create a descriptor set that binds that buffer and update it
	VkDescriptorSet globalDescriptor =
		this->GetCurrentFrame().frameDescriptors.Allocate(this->m_device, this->m_gpuSceneDataDescriptorLayout);

	// Local scope to use another writer later one
	{
		DescriptorWriter writer;
		writer.WriteBuffer(0, static_cast<VkBuffer>(gpuSceneDataBuffer.GetBuffer()), sizeof(GPUSceneData), 0,
						   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
		writer.UpdateSet(this->m_device, globalDescriptor);
	}

	// begin a render pass  connected to our draw image
	// TODO: Remove as if chapter 6
	VkRenderingAttachmentInfo colorAttachment = VkUtilsFactory::CreateAttachmentInfoWithLayout(
		this->m_drawImage.imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	VkRenderingAttachmentInfo depthAttachment =
		VkUtilsFactory::DepthAttachmentInfo(this->m_depthImage.imageView, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL);

	VkExtent2D extent = {this->m_width, this->m_height};

	VkRenderingInfo renderInfo = VkUtilsFactory::CreateRenderingInfo(extent, &colorAttachment, &depthAttachment);
	vkCmdBeginRendering(cmd, &renderInfo);

	// set dynamic viewport and scissor
	VkViewport viewport = {};
	viewport.x = 0;
	viewport.y = 0;
	viewport.width = static_cast<float>(extent.width);
	viewport.height = static_cast<float>(extent.height);
	viewport.minDepth = 0.f;
	viewport.maxDepth = 1.f;

	vkCmdSetViewport(cmd, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	scissor.extent.width = extent.width;
	scissor.extent.height = extent.height;

	vkCmdSetScissor(cmd, 0, 1, &scissor);

	int32_t drawCalls = 0;

	this->DrawGrid(cmd, globalDescriptor);

	auto drawRenderObject = [&](const VkRenderObject &draw) {
		vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->pipeline);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 0, 1,
								&globalDescriptor, 0, nullptr);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, draw.material->pipeline->layout, 1, 1,
								&draw.material->materialSet, 0, nullptr);

		vkCmdBindIndexBuffer(cmd, draw.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		GPUDrawPushConstants pushConstants{};
		pushConstants.vertexBuffer = draw.vertexBufferAddress;
		pushConstants.modelMatrix = draw.transform;
		vkCmdPushConstants(cmd, draw.material->pipeline->layout, VK_SHADER_STAGE_VERTEX_BIT, 0,
						   sizeof(GPUDrawPushConstants), &pushConstants);
		vkCmdDrawIndexed(cmd, draw.indexCount, 1, draw.firstIndex, 0, 0);
		drawCalls++;
	};

	for (const VkRenderObject &draw : this->m_mainDrawContext.opaqueSurfaces)
	{
		drawRenderObject(draw);
	}

	for (const VkRenderObject &draw : this->m_mainDrawContext.transparentSurfaces)
	{
		drawRenderObject(draw);
	}

	////add it to the deletion queue of this frame so it gets deleted once its been used
	this->GetCurrentFrame().deletionQueue.PushFunction(
		[=, this, &gpuSceneDataBuffer]() { gpuSceneDataBuffer.Dispose(m_allocator); });

	vkCmdEndRendering(cmd);
}

void Hush::VulkanRenderer::DrawBackground(VkCommandBuffer cmd) noexcept
{
	// bind the gradient drawing compute pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->m_gradientPipeline);

	// bind the descriptor set containing the draw image for the compute pipeline
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_COMPUTE, this->m_gradientPipelineLayout, 0, 1,
							&this->m_drawImageDescriptors, 0, nullptr);

	ComputePushConstants pc{};
	pc.data1 = glm::vec4(1, 0, 0, 1);
	pc.data2 = glm::vec4(0, 0, 1, 1);
	constexpr uint32_t computeConstantsSize = sizeof(ComputePushConstants);
	vkCmdPushConstants(cmd, this->m_gradientPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, computeConstantsSize, &pc);
	//  execute the compute pipeline dispatch. We are using 16x16 workgroup size so we need to divide by it
	uint32_t roundedWidth = static_cast<uint32_t>(std::ceil(this->m_width / 16.0));
	uint32_t roundedHeight = static_cast<uint32_t>(std::ceil(this->m_height / 16.0));
	vkCmdDispatch(cmd, roundedWidth, roundedHeight, 1);
}

void Hush::VulkanRenderer::DrawGrid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor)
{
	ShaderMaterial *shaderMat = this->m_gridEffect.GetMaterial();
	glm::vec3 cameraPos = this->m_editorCamera.GetPosition();
	glm::mat4 view = this->m_editorCamera.GetViewMatrix();
	glm::mat4 proj = this->m_editorCamera.GetProjectionMatrix();

	proj[1][1] *= -1;

	ShaderMaterial::EError resultCode = ShaderMaterial::EError::None;
	resultCode = shaderMat->SetProperty("farPlane", this->m_editorCamera.GetFarPlane());
	HUSH_ASSERT(resultCode == ShaderMaterial::EError::None, "{}", magic_enum::enum_name(resultCode));

	resultCode = shaderMat->SetProperty("pos", cameraPos);
	HUSH_ASSERT(resultCode == ShaderMaterial::EError::None, "{}", magic_enum::enum_name(resultCode));

	resultCode = shaderMat->SetProperty("viewproj", proj * view);
	HUSH_ASSERT(resultCode == ShaderMaterial::EError::None, "{}", magic_enum::enum_name(resultCode));
	this->m_gridEffect.RecordCommands(cmd, globalDescriptor);
}

void Hush::VulkanRenderer::DrawUI(VkCommandBuffer cmd, VkImageView imageView)
{
	VkRenderingAttachmentInfo colorAttachment =
		VkUtilsFactory::CreateAttachmentInfoWithLayout(imageView, nullptr, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	VkRenderingInfo renderInfo =
		VkUtilsFactory::CreateRenderingInfo(this->m_swapchain.GetExtent(), &colorAttachment, nullptr);

	vkCmdBeginRendering(cmd, &renderInfo);
	auto *uiImpl = dynamic_cast<VulkanImGuiForwarder *>(this->m_uiForwarder.get());
	uiImpl->RenderFrame(cmd);

	vkCmdEndRendering(cmd);
}

VkCommandBuffer Hush::VulkanRenderer::PrepareCommandBuffer(FrameData &currentFrame, uint32_t *swapchainImageIndex)
{
	// wait until the gpu has finished rendering the last frame. Timeout of 1 second
	const uint32_t fenceTargetCount = 1u;
	// Wait until the gpu has finished rendering the last frame. Timeout of 1 second
	VkResult rc =
		vkWaitForFences(this->m_device, fenceTargetCount, &currentFrame.renderFence, true, VK_OPERATION_TIMEOUT_NS);
	currentFrame.deletionQueue.Flush();
	currentFrame.frameDescriptors.ClearPool(this->m_device);
	HUSH_VK_ASSERT(rc, "Fence wait failed!");

	// Request an image from the swapchain
	*swapchainImageIndex =
		this->m_swapchain.AcquireNextImage(currentFrame.swapchainSemaphore, &this->m_resizeRequested);
	// Handle resize request, pass this back to the caller to check
	if (this->m_resizeRequested)
	{
		// Resized the surface, so, skip frame
		return nullptr;
	}
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

	return cmd;
}

void Hush::VulkanRenderer::ResizeSwapchain()
{
	// Defer this to the WindowRenderer interface instead of SDL
	int32_t width, height;
	SDL_GetWindowSize(static_cast<SDL_Window *>(this->m_windowContext), &width, &height);
	this->m_swapchain.Resize(width, height);
	this->m_resizeRequested = false;
}

Hush::GpuAllocatedImage Hush::VulkanRenderer::CreateImage(Hush::ImageExtent3D size, Color::EFormat format,
														  uint32_t usage, bool mipmapped /*= false*/)
{
	GpuAllocatedImage newImage{};
	VkFormat vulkanFormat = this->HushFormatToVkFormat(format);
	newImage.imageFormat = vulkanFormat;
	auto extent = VkExtent3D{size.width, size.height, size.depth};
	newImage.imageExtent = size;

	VkImageCreateInfo imgInfo = VkUtilsFactory::CreateImageCreateInfo(vulkanFormat, usage, extent);
	if (mipmapped)
	{
		imgInfo.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(size.width, size.height)))) + 1;
	}

	// always allocate images on dedicated GPU memory
	VmaAllocationCreateInfo allocinfo = {};
	allocinfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
	allocinfo.requiredFlags = VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// allocate and create the image
	VkResult rc =
		vmaCreateImage(this->m_allocator, &imgInfo, &allocinfo, &newImage.image, &newImage.allocation, nullptr);
	HUSH_VK_ASSERT(rc, "Failed to create Image through 3D extent");

	// if the format is a depth format, we will need to have it use the correct
	// aspect flag
	VkImageAspectFlags aspectFlag = VK_IMAGE_ASPECT_COLOR_BIT;
	if (format == Color::EFormat::D32Float)
	{
		aspectFlag = VK_IMAGE_ASPECT_DEPTH_BIT;
	}

	// build a image-view for the image
	VkImageViewCreateInfo viewInfo =
		VkUtilsFactory::CreateImageViewCreateInfo(vulkanFormat, newImage.image, aspectFlag);
	viewInfo.subresourceRange.levelCount = imgInfo.mipLevels;

	HUSH_VK_ASSERT(vkCreateImageView(this->m_device, &viewInfo, nullptr, &newImage.imageView),
				   "Failed to create image view!");

	return newImage;
}

VkSurfaceKHR Hush::VulkanRenderer::GetSurface() noexcept
{
	return this->m_surface;
}

void Hush::VulkanRenderer::AddToDeletionQueue(std::function<void()> &&deleteFunc)
{
	this->m_mainDeletionQueue.PushFunction(deleteFunc);
}

const Hush::DefaultImageProvider *Hush::VulkanRenderer::GetDefaultImageProvider() const noexcept
{
	return &this->m_defaultImageProvider;
}

Hush::ShaderModuleLoader &Hush::VulkanRenderer::GetShaderModuleLoader() noexcept
{
	return this->m_shaderModuleLoader;
}

void Hush::VulkanRenderer::DestroyImage(Hush::GpuAllocatedImage *img)
{
	vkDestroyImageView(this->m_device, img->imageView, nullptr);
	vmaDestroyImage(this->m_allocator, img->image, img->allocation);
}

constexpr VkFormat Hush::VulkanRenderer::HushFormatToVkFormat(const Hush::Color::EFormat &format)
{

	switch (format)
	{
	case Color::EFormat::R8Unorm:
		return VkFormat::VK_FORMAT_R8_UNORM;
	case Color::EFormat::RG8Unorm:
		return VkFormat::VK_FORMAT_R8G8_UNORM;
	case Color::EFormat::RGB8Unorm:
		return VkFormat::VK_FORMAT_R8G8B8_UNORM;
	case Color::EFormat::RGBA8Unorm:
		return VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
	case Color::EFormat::R16Float:
		return VkFormat::VK_FORMAT_R16_SFLOAT;
	case Color::EFormat::RG16Float:
		return VkFormat::VK_FORMAT_R16G16_SFLOAT;
	case Color::EFormat::RGB16Float:
		return VkFormat::VK_FORMAT_R16G16B16_SFLOAT;
	case Color::EFormat::RGBA16Float:
		return VkFormat::VK_FORMAT_R16G16B16A16_SFLOAT;
	case Color::EFormat::R32Float:
		return VkFormat::VK_FORMAT_R32_SFLOAT;
	case Color::EFormat::RG32Float:
		return VkFormat::VK_FORMAT_R32G32_SFLOAT;
	case Color::EFormat::RGB32Float:
		return VkFormat::VK_FORMAT_R32G32B32_SFLOAT;
	case Color::EFormat::RGBA32Float:
		return VkFormat::VK_FORMAT_R32G32B32A32_SFLOAT;
	case Color::EFormat::D32Float:
		return VkFormat::VK_FORMAT_D32_SFLOAT;
	case Color::EFormat::D24S8Unorm:
		return VkFormat::VK_FORMAT_D24_UNORM_S8_UINT;
	default:
		return VkFormat::VK_FORMAT_UNDEFINED;
	}
}

Hush::GpuAllocatedImage Hush::VulkanRenderer::CreateImage(const void *data, const ImageExtent3D &size,
														  Hush::Color::EFormat format, uint32_t usage,
														  bool mipmapped /*= false*/)
{

	uint32_t dataSize = size.depth * size.width * size.height * 4;
	GpuAllocatedBuffer uploadbuffer(dataSize, GpuAllocatedBuffer::EBufferUsage::TransferSrc,
									GpuAllocatedBuffer::EMemoryUsage::CpuToGpu, this->m_allocator);

	memcpy(uploadbuffer.GetMappedData(), data, dataSize);

	GpuAllocatedImage newImage = this->CreateImage(
		size, format, usage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, mipmapped);

	this->ImmediateSubmit([&](VkCommandBuffer cmd) {
		TransitionImage(cmd, newImage.image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkBufferImageCopy copyRegion = {};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;
		// TODO: Maybe implement this conversion as a reinterpret cast
		copyRegion.imageExtent = {size.width, size.height, size.depth};

		// copy the buffer into the image
		vkCmdCopyBufferToImage(cmd, static_cast<VkBuffer>(uploadbuffer.GetBuffer()), newImage.image,
							   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

		TransitionImage(cmd, newImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
						VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	});
	uploadbuffer.Dispose(this->m_allocator);

	return newImage;
}

Hush::GPUMeshBuffers Hush::VulkanRenderer::UploadMesh(const std::vector<uint32_t> &indices,
													  const std::vector<Mesh::Vertex> &vertices)
{
	const uint32_t vertexBufferSize = static_cast<uint32_t>(vertices.size() * sizeof(Mesh::Vertex));
	const uint32_t indexBufferSize = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));

	GPUMeshBuffers newSurface;

	// create vertex buffer
	newSurface.vertexBuffer = GpuAllocatedBuffer(vertexBufferSize,
												 GpuAllocatedBuffer::EBufferUsage::StorageBuffer |
													 GpuAllocatedBuffer::EBufferUsage::TransferDst |
													 GpuAllocatedBuffer::EBufferUsage::ShaderDeviceAddress,
												 GpuAllocatedBuffer::EMemoryUsage::GpuOnly, this->m_allocator);

	// find the adress of the vertex buffer
	VkBufferDeviceAddressInfo deviceAddressInfo{};
	deviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
	deviceAddressInfo.buffer = static_cast<VkBuffer>(newSurface.vertexBuffer.GetBuffer());
	newSurface.vertexBufferAddress = vkGetBufferDeviceAddress(this->m_device, &deviceAddressInfo);

	// create index buffer
	newSurface.indexBuffer = GpuAllocatedBuffer(
		indexBufferSize, GpuAllocatedBuffer::EBufferUsage::IndexBuffer | GpuAllocatedBuffer::EBufferUsage::TransferDst,
		GpuAllocatedBuffer::EMemoryUsage::GpuOnly, this->m_allocator);

	GpuAllocatedBuffer staging(vertexBufferSize + indexBufferSize, GpuAllocatedBuffer::EBufferUsage::TransferSrc,
							   GpuAllocatedBuffer::EMemoryUsage::CpuOnly, this->m_allocator);

	void *data = staging.GetMappedData();

	// copy vertex buffer
	memcpy(data, vertices.data(), vertexBufferSize);
	// copy index buffer
	memcpy((uint8_t *)data + vertexBufferSize, indices.data(), indexBufferSize);

	this->ImmediateSubmit([&](VkCommandBuffer cmd) {
		VkBufferCopy vertexCopy{0};
		vertexCopy.dstOffset = 0;
		vertexCopy.srcOffset = 0;
		vertexCopy.size = vertexBufferSize;

		vkCmdCopyBuffer(cmd, static_cast<VkBuffer>(staging.GetBuffer()),
						static_cast<VkBuffer>(newSurface.vertexBuffer.GetBuffer()), 1, &vertexCopy);

		VkBufferCopy indexCopy{0};
		indexCopy.dstOffset = 0;
		indexCopy.srcOffset = vertexBufferSize;
		indexCopy.size = indexBufferSize;

		vkCmdCopyBuffer(cmd, static_cast<VkBuffer>(staging.GetBuffer()),
						static_cast<VkBuffer>(newSurface.indexBuffer.GetBuffer()), 1, &indexCopy);
	});

	staging.Dispose(this->m_allocator);

	return newSurface;
}
