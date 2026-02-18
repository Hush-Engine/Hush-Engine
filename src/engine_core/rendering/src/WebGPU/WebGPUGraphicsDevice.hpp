/*! \file WebGPUGraphicsDevice.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU implementation of IGraphicsDevice interface
*/
#pragma once

#include "../IGraphicsDevice.hpp"
#include <webgpu/webgpu.hpp>
#include <memory>
#include <deque>

namespace Hush::Graphics
{
	// ============================================================================
	// WebGPU Resource Implementations
	// ============================================================================

	/// @brief WebGPU buffer implementation
	class WebGPUBuffer : public IGraphicsBuffer
	{
	public:
		WebGPUBuffer(wgpu::Buffer buffer, const BufferDescriptor& desc);
		~WebGPUBuffer() override;

		WebGPUBuffer(const WebGPUBuffer&) = delete;
		WebGPUBuffer(WebGPUBuffer&&) = delete;
		WebGPUBuffer& operator=(const WebGPUBuffer&) = delete;
		WebGPUBuffer& operator=(WebGPUBuffer&&) = delete;

		[[nodiscard]] uint64_t GetSize() const override { return m_descriptor.size; }
		[[nodiscard]] EBufferUsage GetUsage() const override { return m_descriptor.usage; }
		void* Map() override;
		void Unmap() override;
		[[nodiscard]] void* GetNativeHandle() const override;

		[[nodiscard]] wgpu::Buffer GetBuffer() const { return m_buffer; }

	private:
		wgpu::Buffer m_buffer;
		BufferDescriptor m_descriptor;
		void* m_mappedData = nullptr;
	};

	/// @brief WebGPU texture implementation
	class WebGPUTexture : public IGraphicsTexture
	{
	public:
		WebGPUTexture(wgpu::Texture texture, wgpu::TextureView view, const TextureDescriptor& desc);
		~WebGPUTexture() override;

		WebGPUTexture(const WebGPUTexture&) = delete;
        WebGPUTexture(WebGPUTexture&&) = delete;
        WebGPUTexture& operator=(const WebGPUTexture&) = delete;
        WebGPUTexture& operator=(WebGPUTexture&&) = delete;

		[[nodiscard]] uint32_t GetWidth() const override { return m_descriptor.width; }
		[[nodiscard]] uint32_t GetHeight() const override { return m_descriptor.height; }
		[[nodiscard]] uint32_t GetDepth() const override { return m_descriptor.depth; }
		[[nodiscard]] ETextureFormat GetFormat() const override { return m_descriptor.format; }
		[[nodiscard]] uint32_t GetMipLevels() const override { return m_descriptor.mipLevels; }
		[[nodiscard]] void* GetNativeHandle() const override;

		[[nodiscard]] wgpu::Texture GetTexture() const { return m_texture; }
		[[nodiscard]] wgpu::TextureView GetView() const { return m_view; }

	private:
		wgpu::Texture m_texture;
		wgpu::TextureView m_view;
		TextureDescriptor m_descriptor;
	};

	/// @brief WebGPU command queue implementation
	class WebGPUCommandQueue : public ICommandQueue
	{
	public:
		WebGPUCommandQueue(wgpu::Queue queue, EQueueType type);
		~WebGPUCommandQueue() override = default;

		WebGPUCommandQueue(const WebGPUCommandQueue&) = delete;
		WebGPUCommandQueue(WebGPUCommandQueue&&) = delete;
		WebGPUCommandQueue& operator=(const WebGPUCommandQueue&) = delete;
		WebGPUCommandQueue& operator=(WebGPUCommandQueue&&) = delete;

		[[nodiscard]] EQueueType GetQueueType() const override { return m_queueType; }
		void Submit(void* commands) override;
		void WaitIdle() override;
		[[nodiscard]] void* GetNativeHandle() const override;

		[[nodiscard]] wgpu::Queue GetQueue() const { return m_queue; }
		wgpu::CommandEncoder CreateEncoder(const char* label = nullptr);

	private:
		wgpu::Queue m_queue;
		EQueueType m_queueType;
	};

	// ============================================================================
	// WebGPU Graphics Device
	// ============================================================================

	/// @brief WebGPU graphics device implementation
	class WebGPUGraphicsDevice : public IGraphicsDevice
	{
	public:
		/// @brief Constructor
		/// @param windowHandle Platform window handle (SDL_Window*)
		explicit WebGPUGraphicsDevice(void* windowHandle);
		~WebGPUGraphicsDevice() override;

		// Non-copyable, non-movable
		WebGPUGraphicsDevice(const WebGPUGraphicsDevice&) = delete;
		WebGPUGraphicsDevice(WebGPUGraphicsDevice&&) = delete;
		WebGPUGraphicsDevice& operator=(const WebGPUGraphicsDevice&) = delete;
		WebGPUGraphicsDevice& operator=(WebGPUGraphicsDevice&&) = delete;

		// ========================================================================
		// IGraphicsDevice Implementation
		// ========================================================================

		[[nodiscard]] EGraphicsAPI GetAPI() const override { return EGraphicsAPI::WebGPU; }
		[[nodiscard]] Hush::Graphics::GraphicsDeviceCapabilities GetCapabilities() const override;
		[[nodiscard]] bool IsInitialized() const override { return m_initialized; }

		[[nodiscard]] std::shared_ptr<IGraphicsBuffer> CreateBuffer(
			const BufferDescriptor& descriptor) override;

		[[nodiscard]] std::shared_ptr<IGraphicsTexture> CreateTexture(
			const TextureDescriptor& descriptor) override;

		void WriteBuffer(IGraphicsBuffer* buffer, const void* data,
						uint64_t size, uint64_t offset = 0) override;

		void WriteTexture(IGraphicsTexture* texture, const void* data,
						 uint32_t width, uint32_t height, uint32_t mipLevel = 0) override;

		[[nodiscard]] ICommandQueue* GetGraphicsQueue() override { return m_graphicsQueue.get(); }
		[[nodiscard]] ICommandQueue* GetComputeQueue() override { return m_graphicsQueue.get(); }
		[[nodiscard]] ICommandQueue* GetTransferQueue() override { return m_graphicsQueue.get(); }

		void* BeginFrame() override;
		void EndFrame() override;

		void Resize(uint32_t width, uint32_t height) override;

		void AddToDeletionQueue(std::function<void()>&& deleteFunc) override;
		void FlushDeletionQueue() override;

		[[nodiscard]] void* GetNativeHandle() const override;

		// ========================================================================
		// WebGPU-Specific API
		// ========================================================================

		[[nodiscard]] wgpu::Device GetDevice() const { return m_device; }
		[[nodiscard]] wgpu::Instance GetInstance() const { return m_instance; }
		[[nodiscard]] wgpu::Surface GetSurface() const { return m_surface; }
		[[nodiscard]] wgpu::TextureFormat GetSurfaceFormat() const { return m_surfaceFormat; }

	private:
		// ========================================================================
		// Initialization
		// ========================================================================

		void InitializeInstance();
		void InitializeAdapter();
		void InitializeDevice();
		void CreateSurface();
		void ConfigureSurface(uint32_t width, uint32_t height);
		void QueryCapabilities();

		// ========================================================================
		// Conversion Helpers
		// ========================================================================

		[[nodiscard]] static wgpu::BufferUsage ConvertBufferUsage(EBufferUsage usage);
		[[nodiscard]] static wgpu::TextureUsage ConvertTextureUsage(ETextureUsage usage);
		[[nodiscard]] static wgpu::TextureFormat ConvertTextureFormat(ETextureFormat format);

		// ========================================================================
		// Callbacks
		// ========================================================================

		static void OnDeviceError(WGPUErrorType type, char const* message, void* userdata);
		static void OnDeviceLost(WGPUDeviceLostReason reason, char const* message, void* userdata);

	private:
		// WebGPU objects
		wgpu::Instance m_instance;
		wgpu::Adapter m_adapter;
		wgpu::Device m_device;
		wgpu::Surface m_surface;

		// Queues
		std::unique_ptr<WebGPUCommandQueue> m_graphicsQueue;

		// Surface state
		wgpu::TextureFormat m_surfaceFormat = wgpu::TextureFormat::BGRA8Unorm;
		uint32_t m_width = 0;
		uint32_t m_height = 0;
		wgpu::TextureView m_currentFrameView;

		// Device capabilities
		GraphicsDeviceCapabilities m_capabilities;

		// Window context
		void* m_windowHandle = nullptr;

		// Deletion queue
		std::deque<std::function<void()>> m_deletionQueue;

		// State
		bool m_initialized = false;
		bool m_needsResize = false;
	};

} // namespace Hush::Graphics
