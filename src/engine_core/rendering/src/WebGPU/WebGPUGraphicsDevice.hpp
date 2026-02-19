/*! \file WebGPUGraphicsDevice.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU implementation of IGraphicsDevice interface
*/
#pragma once

#include "../RHI/IGraphicsDevice.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "RHI/IGraphicsTexture.hpp"
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
		std::shared_ptr<IGraphicsBuffer> CreateBuffer(const BufferDescriptor &descriptor) override;

		[[nodiscard]]
		std::unique_ptr<IGraphicsTexture> CreateTexture(const TextureDescriptor &descriptor) override;

		[[nodiscard]]
		std::unique_ptr<ICopyCommandList> CreateCopyCommandList() override;

		[[nodiscard]]
		std::unique_ptr<IComputeCommandList> CreateComputeCommandList() override;

		[[nodiscard]]
		std::unique_ptr<IGraphicsCommandList> CreateGraphicsCommandList() override;

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

		void BeginFrame() override;
		void EndFrame() override;

		[[nodiscard]]
		IGraphicsTexture* GetCurrentFrameTexture() const override;

		void Resize(uint32_t width, uint32_t height) override;

		void AddToDeletionQueue(std::function<void()> &&deleteFunc) override;
		void FlushDeletionQueue() override;

		[[nodiscard]]
		void *GetNativeHandle() const override;

		// ========================================================================
		// WebGPU-Specific API
		// ========================================================================

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
