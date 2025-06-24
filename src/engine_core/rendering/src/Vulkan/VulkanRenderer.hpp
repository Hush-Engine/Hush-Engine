/*! \file VulkanRenderer.hpp
	\author Alan Ramirez Herrera
	\date 2024-03-03
	\brief Vulkan implementation for rendering
*/

#pragma once
#include <utility>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define VK_NO_PROTOTYPES

#include "Renderer.hpp"
#include <magic_enum/magic_enum.hpp>
#include "FrameData.hpp"
#include "VulkanDeletionQueue.hpp"
#include "ImGui/IImGuiForwarder.hpp"
#include "vk_mem_alloc.hpp"
#include <VkBootstrap.h>
#include <array>
#include <functional>
#include <vector>
#include <vulkan/vulkan.h>
#include "VkDescriptors.hpp"
#include "GPUSceneData.hpp"
#include "GltfMetallicRoughness.hpp"
#include "Shared/EditorCamera.hpp"
#include "VulkanSwapchain.hpp"
#include "Vulkan/ShaderModuleLoader.hpp"
#include "VulkanFullScreenPass.hpp"
#include "DrawContext.hpp"
#include "Shared/Mesh.hpp"
#include "Shared/GpuAllocatedImage.hpp"
#include "Shared/Types/Color.hpp"
#include "Shared/DefaultImages.hpp"
#include <cstdint>

///@brief Double frame buffering, allows for the GPU and CPU to work in parallel. NOTE: increase to 3 if experiencing
/// jittery framerates
constexpr uint32_t FRAME_OVERLAP = 2;

constexpr uint32_t VK_OPERATION_TIMEOUT_NS = 1'000'000'000; // This is one second, trust me (1E-9)

namespace Hush
{
	struct MeshAsset;
	struct DirectionalLight;
	struct WorldTransform;

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

		void SetActiveScene(Scene *scene) override;

		void CreateSwapChain(uint32_t width, uint32_t height) override;

		void InitRendering() override;

		void InitializeCommands() noexcept;

		void InitImGui() override;

		void PushMesh(const glm::mat4 &globalTransform, std::shared_ptr<Mesh> mesh) override;

		void DestroyMesh(const std::string_view &name) override;

		void Draw(float delta) override;

		void NewUIFrame() const noexcept override;

		void EndUIFrame() const noexcept override;

		void HandleEvent(const SDL_Event *event) noexcept override;

		void UpdateSceneObjects(float delta) override;

		void Dispose();

		void ImmediateSubmit(std::function<void(VkCommandBuffer cmd)> &&function) noexcept;

		FrameData &GetCurrentFrame() noexcept;

		FrameData &GetLastFrame() noexcept;

		/* CONSTANT GETTERS */

		[[nodiscard]]
		VkSampler GetDefaultSamplerLinear() noexcept;

		[[nodiscard]]
		VkSampler GetDefaultSamplerNearest() noexcept;

		[[nodiscard]]
		GLTFMetallicRoughness &GetMetalRoughMaterial() noexcept;

		[[nodiscard]]
		DescriptorAllocatorGrowable &GlobalDescriptorAllocator() noexcept;

		[[nodiscard]]
		VmaAllocator GetVmaAllocator() noexcept;

		[[nodiscard]]
		VkInstance GetVulkanInstance() noexcept;

		[[nodiscard]]
		VkDevice GetVulkanDevice() noexcept;

		[[nodiscard]]
		VkDescriptorSetLayout GetGpuSceneDataDescriptorLayout() noexcept;

		[[nodiscard]]
		const GpuAllocatedImage &GetDrawImage() const noexcept;

		// Non const variant
		[[nodiscard]]
		GpuAllocatedImage &GetDrawImage() noexcept;

		[[nodiscard]]
		const GpuAllocatedImage &GetDepthImage() const noexcept;

		// Non const variant
		[[nodiscard]]
		GpuAllocatedImage &GetDepthImage() noexcept;

		[[nodiscard]]
		VkPhysicalDevice GetVulkanPhysicalDevice() const noexcept;

		[[nodiscard]]
		VkQueue GetGraphicsQueue() const noexcept;

		[[nodiscard]]
		void *GetWindowContext() const noexcept override;

		void SetDirectionalLight(DirectionalLight *light) noexcept override;

		[[nodiscard]]
		const EditorCamera &GetEditorCamera() const noexcept override;

		VulkanSwapchain &GetSwapchain();

		GPUMeshBuffers UploadMesh(const std::vector<uint32_t> &indices, const std::vector<Mesh::Vertex> &vertices);

		// GpuAllocatedImage CreateImage(const void *data, VkExtent3D size, VkFormat format, VkImageUsageFlags usage,
		// 						   bool mipmapped = false);

		GpuAllocatedImage CreateImage(const void *data, const ImageExtent3D &size, Color::EFormat format,
									  uint32_t usage, bool mipmapped = false) override;

		VkSurfaceKHR GetSurface() noexcept;

		void AddToDeletionQueue(std::function<void()> &&deleteFunc) override;

		[[nodiscard]]
		const DefaultImageProvider *GetDefaultImageProvider() const noexcept override;

		[[nodiscard]]
		ShaderModuleLoader &GetShaderModuleLoader() noexcept;

		EditorCamera* GetEditorCamera() noexcept override;
		
	private:
		void Configure(vkb::Instance vkbInstance);

		void CreateSyncObjects();

		VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signalSemaphoreInfo,
								 VkSemaphoreSubmitInfo *waitSemaphoreInfo);

		void LoadDebugMessenger();

		static uint32_t LogDebugMessage(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
										VkDebugUtilsMessageTypeFlagsEXT messageTypes,
										const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData);

		void InitVmaAllocator();

		void InitRenderables();

		void TransitionImage(VkCommandBuffer cmd, VkImage image, VkImageLayout currentLayout, VkImageLayout newLayout);

		void CopyImageToImage(VkCommandBuffer cmd, VkImage source, VkImage destination, VkExtent2D srcSize,
							  VkExtent2D dstSize);

		void InitDescriptors() noexcept;

		void InitPipelines() noexcept;

		void InitBackgroundPipelines() noexcept;

		void InitMeshPipeline() noexcept;

		void InitDefaultData() noexcept;

		void DrawGeometry(VkCommandBuffer cmd);

		void DrawBackground(VkCommandBuffer cmd) noexcept;

		void DrawGrid(VkCommandBuffer cmd, VkDescriptorSet globalDescriptor);

		void DrawUI(VkCommandBuffer cmd, VkImageView imageView);

		VkCommandBuffer PrepareCommandBuffer(FrameData &currentFrame, uint32_t *swapchainImageIndex);

		void ResizeSwapchain();

		// GpuAllocatedImage CreateImage(VkExtent3D size, VkFormat format, VkImageUsageFlags usage, bool mipmapped =
		// false);

		GpuAllocatedImage CreateImage(ImageExtent3D size, Color::EFormat format, uint32_t usage,
									  bool mipmapped = false);

		void DestroyImage(GpuAllocatedImage *img) override;

		constexpr VkFormat HushFormatToVkFormat(const Color::EFormat &format);

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
		VkDescriptorSet m_drawImageDescriptors = nullptr;
		VkDescriptorSetLayout m_drawImageDescriptorLayout = nullptr;
		VkPipeline m_gradientPipeline = nullptr;
		VkPipelineLayout m_gradientPipelineLayout = nullptr;
		VkPipelineLayout m_trianglePipelineLayout = nullptr;
		VkPipeline m_trianglePipeline = nullptr;
		VkPipelineLayout m_meshPipelineLayout = nullptr;
		VkPipeline m_meshPipeline = nullptr;
		GPUSceneData m_sceneData;
		VkDescriptorSetLayout m_gpuSceneDataDescriptorLayout;

		GPUMeshBuffers m_rectangle;

		uint32_t m_graphicsQueueFamily = 0u;
		DescriptorAllocatorGrowable m_globalDescriptorAllocator{};

		VkExtent2D m_drawExtent{};
		float m_renderScale = 1.0f;
		uint32_t m_width = 0u;
		uint32_t m_height = 0u;
		// draw resources
		GpuAllocatedImage m_drawImage{};
		GpuAllocatedImage m_depthImage{};

		DrawContext m_mainDrawContext;
		std::vector<std::pair<WorldTransform *, Mesh *>> m_loadedMeshes;
		// Test stuff
		DefaultImageProvider m_defaultImageProvider;
		ShaderModuleLoader m_shaderModuleLoader{};
		GpuAllocatedImage m_greyImage{};
		GpuAllocatedImage m_errorCheckerboardImage{};
		VkDescriptorSetLayout m_singleImageDescriptorLayout;

		GLTFMetallicRoughness m_metalRoughMaterial;
		VulkanFullScreenPass m_gridEffect;

		VkSampler m_defaultSamplerLinear;
		VkSampler m_defaultSamplerNearest;

		EditorCamera m_editorCamera;
		Scene *m_activeScene = nullptr;
		DirectionalLight *m_directionalLight = nullptr;
		WorldTransform *m_sunTransform = nullptr;

		// Frame related data
		std::array<FrameData, FRAME_OVERLAP> m_frames{};
		// Frame counter
		//(This should run fine for like, 414 days at 60 fps, and 69 days at like 360 fps)
		int m_frameNumber = 0;
		std::unique_ptr<IImGuiForwarder> m_uiForwarder = nullptr;

		VulkanDeletionQueue m_mainDeletionQueue{};
		VmaAllocator m_allocator = nullptr; // vma lib allocator
		bool m_resizeRequested = false;
		VulkanSwapchain m_swapchain{};
	};
} // namespace Hush
