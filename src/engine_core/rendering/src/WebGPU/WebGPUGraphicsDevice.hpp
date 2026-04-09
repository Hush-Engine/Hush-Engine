/*! \file WebGPUGraphicsDevice.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU implementation of IGraphicsDevice interface
*/
#pragma once

#include "../RHI/IGraphicsDevice.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "RHI/IGraphicsTexture.hpp"
#include "RHI/ISampler.hpp"
#include "RHI/ShaderCompiler.hpp"
#include "WebGPUCommandQueue.hpp"
#include "WebGPUTexture.hpp"
#include <webgpu/webgpu.hpp>
#include <memory>
#include <deque>
#include <functional>

namespace Hush::Graphics
{
	/// @brief WebGPU graphics device implementation
	class WebGPUGraphicsDevice : public IGraphicsDevice
	{
	public:
		/// @brief Constructor
		/// @param windowHandle Platform window handle (SDL_Window*)
		explicit WebGPUGraphicsDevice(void *windowHandle);
		~WebGPUGraphicsDevice() override;

		// Non-copyable, non-movable
		WebGPUGraphicsDevice(const WebGPUGraphicsDevice &) = delete;
		WebGPUGraphicsDevice(WebGPUGraphicsDevice &&) = delete;
		WebGPUGraphicsDevice &operator=(const WebGPUGraphicsDevice &) = delete;
		WebGPUGraphicsDevice &operator=(WebGPUGraphicsDevice &&) = delete;

		[[nodiscard]]
		EGraphicsAPI GetAPI() const override
		{
			return EGraphicsAPI::WebGPU;
		}
		[[nodiscard]]
		Hush::Graphics::GraphicsDeviceCapabilities GetCapabilities() const override;
		[[nodiscard]]
		bool IsInitialized() const override
		{
			return m_initialized;
		}

		[[nodiscard]]
		std::unique_ptr<IGraphicsBuffer> CreateBuffer(const BufferDescriptor &descriptor) override;

		void WriteBuffer(IGraphicsBuffer *buffer, uint64_t offset, const void *data, uint64_t size) override;

		[[nodiscard]]
		std::unique_ptr<IGraphicsTexture> CreateTexture(const TextureDescriptor &descriptor) override;

		[[nodiscard]]
		std::unique_ptr<ISampler> CreateSampler(const SamplerDescriptor &descriptor) override;

		[[nodiscard]]
		std::unique_ptr<IShaderModule> CreateShaderModule(const ShaderModuleDescriptor &descriptor) override;

		[[nodiscard]]
		std::unique_ptr<IGraphicsPipeline> CreateGraphicsPipeline(
			const GraphicsPipelineDescriptor &descriptor) override;

		[[nodiscard]]
		std::unique_ptr<IComputePipeline> CreateComputePipeline(const ComputePipelineDescriptor &descriptor) override;

		[[nodiscard]]
		std::unique_ptr<IBindGroupLayout> CreateBindGroupLayout(const BindGroupLayoutDescriptor &descriptor) override;

		[[nodiscard]]
		std::unique_ptr<IBindGroup> CreateBindGroup(const BindGroupDescriptor &descriptor) override;

		[[nodiscard]]
		std::unique_ptr<ICopyCommandList> CreateCopyCommandList() override;

		[[nodiscard]]
		std::unique_ptr<IComputeCommandList> CreateComputeCommandList() override;

		[[nodiscard]]
		std::unique_ptr<IGraphicsCommandList> CreateGraphicsCommandList() override;

		[[nodiscard]]
		std::unique_ptr<IFence> CreateFence(uint64_t initialValue = 0) override;

		[[nodiscard]]
		ICommandQueue *GetGraphicsQueue() override
		{
			return m_graphicsQueue.get();
		}

		[[nodiscard]]
		ICommandQueue *GetComputeQueue() override
		{
			return m_graphicsQueue.get();
		}

		[[nodiscard]]
		ICommandQueue *GetTransferQueue() override
		{
			return m_graphicsQueue.get();
		}

		/// WebGPU exposes a single queue — collapse all pass types onto queue 0
		/// so the render graph never generates cross-queue sync, fences, or
		/// separate execution plans.
		[[nodiscard]]
		uint32_t MapPassTypeToQueueIndex([[maybe_unused]] EQueueType passType) const override
		{
			return 0;
		}

		void BeginFrame() override;
		void EndFrame() override;

		[[nodiscard]]
		IGraphicsTexture *GetCurrentFrameTexture() const override;

		void Resize(uint32_t width, uint32_t height) override;

		void AddToDeletionQueue(std::function<void()> &&deleteFunc) override;
		void FlushDeletionQueue() override;

		[[nodiscard]]
		void *GetNativeHandle() const override;

		[[nodiscard]]
		wgpu::Device GetDevice() const
		{
			return m_device;
		}

		[[nodiscard]]
		wgpu::Instance GetInstance() const
		{
			return m_instance;
		}

		[[nodiscard]]
		wgpu::Surface GetSurface() const
		{
			return m_surface;
		}

		[[nodiscard]]
		wgpu::TextureFormat GetSurfaceFormat() const
		{
			return m_surfaceFormat;
		}

		void PollEvents();

		ETextureFormat GetPreferredSwapchainFormat() const override;

	private:
		void InitializeInstance();
		void InitializeAdapter();
		void InitializeDevice();
		void CreateSurface();
		void ConfigureSurface(uint32_t width, uint32_t height);
		void QueryCapabilities();

		[[nodiscard]]
		static wgpu::BufferUsage ConvertBufferUsage(EBufferUsage usage);
		[[nodiscard]]
		static wgpu::TextureUsage ConvertTextureUsage(ETextureUsage usage);
		[[nodiscard]]
		static wgpu::TextureFormat ConvertTextureFormat(ETextureFormat format);

		[[nodiscard]]
		static ETextureFormat ConvertToEngineTextureFormat(wgpu::TextureFormat format);

		static void OnDeviceError(WGPUErrorType type, char const *message, void *userdata);
		static void OnDeviceLost(WGPUDeviceLostReason reason, char const *message, void *userdata);

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

		mutable WebGPUTexture m_currentFrameTexture;

		// Device capabilities
		GraphicsDeviceCapabilities m_capabilities;

		// Window context
		void *m_windowHandle = nullptr;

		// Deletion queue
		std::deque<std::function<void()>> m_deletionQueue;

		// State
		bool m_initialized = false;
		bool m_needsResize = false;
	};

} // namespace Hush::Graphics
