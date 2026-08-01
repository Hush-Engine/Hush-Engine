/*! \file WebGPUGraphicsDevice.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU implementation of IGraphicsDevice interface
*/
#include "WebGPUGraphicsDevice.hpp"
#include "BitwiseUtils.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "RHI/IGraphicsTexture.hpp"
#include "WebGPU/WebGPUTexture.hpp"
#include "WebGPUBuffer.hpp"
#include "WebGPUCommandList.hpp"
#include "WebGPUFence.hpp"
#include "WebGPUShaderModule.hpp"
#include "WebGPUPipeline.hpp"
#include "WebGPUBindGroup.hpp"
#include "WebGPUSampler.hpp"
#include "Logger.hpp"
#include "Assertions.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>
#include <cstdint>
#include <sdl3webgpu/sdl3webgpu.h>
#include <magic_enum/magic_enum.hpp>
#include <webgpu/webgpu.hpp>
#include "Profiling.hpp"

#if HUSH_PLATFORM_EMSCRIPTEN
#include <emscripten/html5.h>
#endif

namespace Hush::Graphics
{

	WebGPUGraphicsDevice::WebGPUGraphicsDevice(void *windowHandle)
		: m_windowHandle(windowHandle)
	{
		ZoneScoped;
		LogTrace("Initializing WebGPU Graphics Device");

		InitializeInstance();
		InitializeAdapter();
		InitializeDevice();
		CreateSurface();

		// Get initial window size
		auto *window = static_cast<SDL_Window *>(m_windowHandle);
		int width = 0;
		int height = 0;
		SDL_GetWindowSize(window, &width, &height);
		Hush::LogFormat(Hush::ELogLevel::Info, "Initial window size: {}x{}", width, height);
		ConfigureSurface(static_cast<uint32_t>(width), static_cast<uint32_t>(height));

		// Create command queue
		wgpu::Queue queue = m_device.getQueue();
		m_graphicsQueue = std::make_unique<WebGPUCommandQueue>(queue, EQueueType::Graphics);

		// Query capabilities
		QueryCapabilities();

		LogTrace("WebGPU Graphics Device partially initialized (stub implementation)");
	}

	WebGPUGraphicsDevice::~WebGPUGraphicsDevice()
	{
		LogTrace("Destroying WebGPU Graphics Device");

		FlushDeletionQueue();

		m_graphicsQueue.reset();
		m_currentFrameView = nullptr;

		if (static_cast<WGPUSurface>(m_surface) != nullptr)
		{
			m_surface.unconfigure();
		}

		m_surface = nullptr;
		m_device = nullptr;
		m_adapter = nullptr;
		m_instance = nullptr;

		LogTrace("WebGPU Graphics Device destroyed");
	}

	void WebGPUGraphicsDevice::InitializeInstance()
	{
		wgpu::InstanceDescriptor instanceDesc{};
		m_instance = wgpu::createInstance(instanceDesc);

		HUSH_ASSERT(m_instance, "Failed to create WebGPU instance");
		LogTrace("WebGPU instance created");
	}

	void WebGPUGraphicsDevice::InitializeAdapter()
	{
		wgpu::RequestAdapterOptions adapterOptions{};
		adapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;

		m_adapter = m_instance.requestAdapter(adapterOptions);
		HUSH_ASSERT(m_adapter, "Failed to request WebGPU adapter");
		LogTrace("WebGPU adapter acquired");
	}

	void WebGPUGraphicsDevice::InitializeDevice()
	{
		wgpu::DeviceDescriptor deviceDesc{};
		deviceDesc.label = wgpu::StringView("Hush Graphics Device");
		m_device = m_adapter.requestDevice(deviceDesc);
		HUSH_ASSERT(m_device, "Failed to request WebGPU device");

		LogTrace("WebGPU device initialization skipped (stub)");
	}

	void WebGPUGraphicsDevice::CreateSurface()
	{
		if (m_surface != nullptr)
		{
			m_surface.release();
			m_surface = nullptr;
		}

		auto *window = static_cast<SDL_Window *>(m_windowHandle);
		WGPUSurface surfaceHandle = SDL_GetWGPUSurface(static_cast<WGPUInstance>(m_instance), window);
		HUSH_ASSERT(surfaceHandle, "Failed to create WebGPU surface");

		m_surface = wgpu::Surface(surfaceHandle);
		LogTrace("WebGPU surface created");
	}

	void WebGPUGraphicsDevice::ConfigureSurface(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0)
		{
			LogWarn("Attempted to configure surface with zero dimensions");
			return;
		}

		m_width = width;
		m_height = height;

		wgpu::SurfaceConfiguration config{};
		config.device = m_device;
		config.format = m_surfaceFormat;
		config.usage = wgpu::TextureUsage::RenderAttachment | wgpu::TextureUsage::CopyDst;
		config.width = width;
		config.height = height;
		config.presentMode = wgpu::PresentMode::Fifo;
		config.alphaMode = wgpu::CompositeAlphaMode::Opaque;

		m_surface.configure(config);

		m_needsResize = false;
	}

	void WebGPUGraphicsDevice::QueryCapabilities()
	{
		wgpu::Limits limits{};
		m_device.getLimits(&limits);

		m_capabilities.maxBufferSize = limits.maxBufferSize;
		m_capabilities.maxTextureDimension2D = limits.maxTextureDimension2D;
		m_capabilities.maxTextureDimension3D = limits.maxTextureDimension3D;
		m_capabilities.maxTextureArrayLayers = limits.maxTextureArrayLayers;
		m_capabilities.maxUniformBufferBindingSize = limits.maxUniformBufferBindingSize;
		m_capabilities.maxStorageBufferBindingSize = limits.maxStorageBufferBindingSize;
		m_capabilities.maxColorAttachments = limits.maxColorAttachments;
		m_capabilities.supportsCompute = true;		   // WebGPU always supports compute
		m_capabilities.supportsGeometryShader = false; // Not in WebGPU
		m_capabilities.supportsTessellation = false;   // Not in WebGPU
		m_capabilities.supportsRayTracing = false;	   // Not in WebGPU

		// WebGPU exposes a single queue — there are no dedicated async compute
		// or transfer queues.  The render graph's queue mapper (see
		// MapPassTypeToQueueIndex override) collapses all pass types onto
		// queue index 0 so that no cross-queue synchronisation is generated.
		m_capabilities.hasAsyncComputeQueue = false;
		m_capabilities.hasDedicatedTransferQueue = false;

		// WebGPU has no native timeline/monotonic fence API.  The engine
		// emulates timeline semantics on the CPU side (see WebGPUFence).
		m_capabilities.supportsTimelineFences = false;
	}

	Hush::Graphics::GraphicsDeviceCapabilities WebGPUGraphicsDevice::GetCapabilities() const
	{
		return m_capabilities;
	}

	wgpu::Buffer WebGPUGraphicsDevice::CreateBufferInternal(const BufferDescriptor &descriptor)
	{
		ZoneScoped;
		wgpu::BufferDescriptor desc{};
		desc.size = descriptor.size;
		desc.usage = ConvertBufferUsage(descriptor.usage);

		// WebGPU requires CopyDst usage for queue.writeBuffer() to work.
		// If the caller requested CPU-writable memory, ensure CopyDst is set
		// so that WriteBuffer() calls succeed.
		if (HasFlag(descriptor.memoryAccess, EMemoryAccess::CPUWrite))
		{
			desc.usage = desc.usage | wgpu::BufferUsage::CopySrc | wgpu::BufferUsage::MapWrite;
		}
		else if (HasFlag(descriptor.memoryAccess, EMemoryAccess::CPURead))
		{
			desc.usage = desc.usage | wgpu::BufferUsage::CopyDst | wgpu::BufferUsage::MapRead;
		}

		desc.mappedAtCreation = static_cast<WGPUBool>(false);

		if (Bitwise::HasCompositeFlag(descriptor.usage, EBufferUsage::Uniform))
		{
			desc.usage |= wgpu::BufferUsage::CopyDst;
		}

		if (descriptor.debugName != nullptr)
		{
			desc.label = wgpu::StringView(descriptor.debugName);
		}

		wgpu::Buffer buffer = m_device.createBuffer(desc);
		if (buffer == nullptr)
		{
			LogError("Failed to create WebGPU buffer");
		}

		return buffer;
	}

	std::unique_ptr<IGraphicsBuffer> WebGPUGraphicsDevice::CreateBuffer(const BufferDescriptor &descriptor)
	{
		wgpu::Buffer buffer = this->CreateBufferInternal(descriptor);
		if (buffer == nullptr)
		{
			// Error is logged on the internal function
			return nullptr;
		}

		return std::make_unique<WebGPUBuffer>(buffer, m_instance, descriptor);
	}

	void WebGPUGraphicsDevice::WriteBuffer(IGraphicsBuffer *buffer, uint64_t offset, const void *data, uint64_t size)
	{
		if (buffer == nullptr || data == nullptr || size == 0)
		{
			return;
		}

		auto *webgpuBuffer = dynamic_cast<WebGPUBuffer *>(buffer);
		if (webgpuBuffer == nullptr)
		{
			LogError("WriteBuffer: buffer is not a WebGPU buffer");
			return;
		}

		wgpu::Queue queue = m_device.getQueue();
		queue.writeBuffer(webgpuBuffer->GetBuffer(), offset, data, static_cast<size_t>(size));
	}

	void WebGPUGraphicsDevice::ResizeBuffer(IGraphicsBuffer *buffer, uint64_t size)
	{
		HUSH_ASSERT(size <= this->m_capabilities.maxBufferSize,
					"Cannot resize buffer size to {}, maximum buffer size for this graphics device is: {}", size,
					this->m_capabilities.maxBufferSize);
		// There is an available descriptor, we just copy that, obviously this is an intentional copy-op
		BufferDescriptor newDesc = buffer->GetDescriptor();
		HUSH_ASSERT(
			Bitwise::HasCompositeFlag(newDesc.usage, EBufferUsage::CopySource),
			"For a buffer to be resized it needs to have the CopySource usage flag, current buffer's usage flags: {}",
			static_cast<uint32_t>(newDesc.usage));
		newDesc.size = size;

		// Copy the original buffer back to the new one
		// Create a new buffer with the updated size and map it
		wgpu::Buffer buff = this->CreateBufferInternal(newDesc);
		uint64_t oldBuffSize = buffer->GetSize();

		if (buff == nullptr)
		{
			LogFormat(ELogLevel::Error, "Failed to resize buffer on buffer creation");
			return;
		}

		if (oldBuffSize > 0)
		{
			uint64_t bytesToCopy = std::min(size, oldBuffSize);
			// Handle alignment for WebGPU (multiples of 4 rounding down to avoid writing out of bounds)
			uint64_t remainder = bytesToCopy % 4;
			if (remainder != 0)
			{
				// Maybe we should issue a warning for the round down(? TBD on PR
				bytesToCopy = (bytesToCopy / 4) * 4;
			}

			// Encoder stuff, we do this raw instead of going through the abstractions
			// because it is easier and we don't mess with lifetimes
			wgpu::CommandEncoder encoder = this->m_device.createCommandEncoder();
			wgpu::Buffer oldBuffer = static_cast<WGPUBuffer>(buffer->GetNativeHandle());
			encoder.copyBufferToBuffer(oldBuffer, 0, buff, 0, bytesToCopy);

			wgpu::CommandBuffer cmdBuffer = encoder.finish();
			wgpu::Queue nativeQueue = static_cast<WGPUQueue>(this->m_graphicsQueue->GetNativeHandle());

			nativeQueue.submit(cmdBuffer);
		}

		// Destroy the current buffer
		buffer->Destroy();

		// Just set the data
		auto *bufferImpl = dynamic_cast<WebGPUBuffer *>(buffer);
		bufferImpl->m_buffer = buff;
		bufferImpl->m_instance = this->m_instance;
		bufferImpl->m_descriptor = newDesc;
	}

	std::unique_ptr<IGraphicsTexture> WebGPUGraphicsDevice::CreateTexture(const TextureDescriptor &descriptor)
	{
		ZoneScoped;
		wgpu::TextureDescriptor desc{};
		desc.size.width = descriptor.width;
		desc.size.height = descriptor.height;
		desc.size.depthOrArrayLayers = descriptor.arrayLayers;
		desc.format = ConvertTextureFormat(descriptor.format);
		desc.usage = ConvertTextureUsage(descriptor.usage);
		desc.dimension = wgpu::TextureDimension::_2D;
		desc.mipLevelCount = descriptor.mipLevels;
		desc.sampleCount = descriptor.sampleCount;

		if (descriptor.debugName != nullptr)
		{
			desc.label = wgpu::StringView(descriptor.debugName);
		}

		wgpu::Texture texture = m_device.createTexture(desc);

		if (texture != nullptr)
		{
			wgpu::TextureViewDescriptor viewDesc = {};
			viewDesc.format = ConvertTextureFormat(descriptor.format);
			viewDesc.dimension =
				(descriptor.arrayLayers > 1) ? wgpu::TextureViewDimension::_2DArray : wgpu::TextureViewDimension::_2D;
			viewDesc.baseMipLevel = 0;
			viewDesc.mipLevelCount = descriptor.mipLevels;
			viewDesc.baseArrayLayer = 0;
			viewDesc.arrayLayerCount = descriptor.arrayLayers;

			wgpu::TextureView textureView = texture.createView(viewDesc);

			return std::make_unique<Hush::Graphics::WebGPUTexture>(texture, textureView, descriptor);
		}

		return nullptr;
	}

	std::unique_ptr<ISampler> WebGPUGraphicsDevice::CreateSampler(const SamplerDescriptor &descriptor)
	{
		auto sampler = std::make_unique<WebGPUSampler>(m_device, descriptor);
		if (sampler->GetNativeHandle() == nullptr)
		{
			LogError("WebGPUGraphicsDevice: Failed to create sampler");
			return nullptr;
		}
		return sampler;
	}

	std::unique_ptr<IShaderModule> WebGPUGraphicsDevice::CreateShaderModule(const ShaderModuleDescriptor &descriptor)
	{
		ZoneScoped;
		auto module = std::make_unique<WebGPUShaderModule>(m_device, descriptor);
		if (!module->IsValid())
		{
			LogError("WebGPUGraphicsDevice: Failed to create shader module");
			return nullptr;
		}
		return module;
	}

	std::unique_ptr<IGraphicsPipeline> WebGPUGraphicsDevice::CreateGraphicsPipeline(
		const GraphicsPipelineDescriptor &descriptor)
	{
		ZoneScoped;
		auto pipeline = std::make_unique<WebGPUGraphicsPipeline>(m_device, descriptor);
		if (!pipeline->IsValid())
		{
			LogError("WebGPUGraphicsDevice: Failed to create graphics pipeline");
			return nullptr;
		}
		return pipeline;
	}

	std::unique_ptr<IComputePipeline> WebGPUGraphicsDevice::CreateComputePipeline(
		const ComputePipelineDescriptor &descriptor)
	{
		ZoneScoped;
		auto pipeline = std::make_unique<WebGPUComputePipeline>(m_device, descriptor);
		if (!pipeline->IsValid())
		{
			LogError("WebGPUGraphicsDevice: Failed to create compute pipeline");
			return nullptr;
		}
		return pipeline;
	}

	std::unique_ptr<IBindGroupLayout> WebGPUGraphicsDevice::CreateBindGroupLayout(
		const BindGroupLayoutDescriptor &descriptor)
	{
		auto layout = std::make_unique<WebGPUBindGroupLayout>(m_device, descriptor);
		if (!layout->IsValid())
		{
			LogError("WebGPUGraphicsDevice: Failed to create bind group layout");
			return nullptr;
		}
		return layout;
	}

	std::unique_ptr<IBindGroup> WebGPUGraphicsDevice::CreateBindGroup(const BindGroupDescriptor &descriptor)
	{
		auto bindGroup = std::make_unique<WebGPUBindGroup>(m_device, descriptor);
		if (!bindGroup->IsValid())
		{
			LogError("WebGPUGraphicsDevice: Failed to create bind group");
			return nullptr;
		}
		return bindGroup;
	}

	std::unique_ptr<ICopyCommandList> WebGPUGraphicsDevice::CreateCopyCommandList()
	{
		return std::make_unique<WebGPUCopyCommandList>(m_device);
	}

	std::unique_ptr<IComputeCommandList> WebGPUGraphicsDevice::CreateComputeCommandList()
	{
		return std::make_unique<WebGPUComputeCommandList>(m_device);
	}

	std::unique_ptr<IGraphicsCommandList> WebGPUGraphicsDevice::CreateGraphicsCommandList()
	{
		return std::make_unique<WebGPUGraphicsCommandList>(m_device);
	}

	std::unique_ptr<IFence> WebGPUGraphicsDevice::CreateFence(uint64_t initialValue)
	{
		return std::make_unique<WebGPUFence>(initialValue);
	}

	void WebGPUGraphicsDevice::BeginFrame()
	{
		m_instance.processEvents();
#ifndef HUSH_PLATFORM_EMSCRIPTEN
		ZoneScoped;
#endif
		if (m_needsResize)
		{
			// Release stale frame texture/view from the previous frame before
			// reconfiguring.  The old surface texture belongs to the old
			// configuration and must not be referenced after configure().
			m_currentFrameView = nullptr;
			m_currentFrameTexture = WebGPUTexture();

			auto *window = static_cast<SDL_Window *>(m_windowHandle);
			int width = 0;
			int height = 0;
			SDL_GetWindowSize(window, &width, &height);
			if (width > 0 && height > 0)
			{
				ConfigureSurface(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
			}
		}

		wgpu::SurfaceTexture surfaceTexture;
		m_surface.getCurrentTexture(&surfaceTexture);

		if (surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessOptimal &&
			surfaceTexture.status != wgpu::SurfaceGetCurrentTextureStatus::SuccessSuboptimal)
		{
			LogError("Failed to acquire surface texture");

			if (surfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::Timeout ||
				surfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::Outdated ||
				surfaceTexture.status == wgpu::SurfaceGetCurrentTextureStatus::Lost)
			{
				m_needsResize = true;
			}

			return;
		}

		wgpu::Texture texture = surfaceTexture.texture;
		m_currentFrameView = texture.createView();

		m_currentFrameTexture = WebGPUTexture(texture, m_currentFrameView,
											  TextureDescriptor{
												  .width = m_width,
												  .height = m_height,
												  .depth = 1,
												  .mipLevels = 1,
												  .sampleCount = 1,
												  .format = ConvertToEngineTextureFormat(m_surfaceFormat),
												  .usage = ETextureUsage::RenderTarget | ETextureUsage::CopySource,
												  .debugName = "Current Frame Texture",
												  .ownedByExternalSource = true,
											  });
	}

	void WebGPUGraphicsDevice::EndFrame()
	{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
		ZoneScoped;
#endif
#if HUSH_PLATFORM_EMSCRIPTEN
		m_currentFrameTexture = WebGPUTexture();
		emscripten_sleep(0);
#else
		m_surface.present();
#endif
		m_currentFrameView = nullptr;
		FlushDeletionQueue();
	}

	IGraphicsTexture *WebGPUGraphicsDevice::GetCurrentFrameTexture() const
	{
		return &m_currentFrameTexture;
	}

	void WebGPUGraphicsDevice::Resize(uint32_t width, uint32_t height)
	{
		if (width == m_width && height == m_height)
		{
			return;
		}

		// Don't reconfigure immediately — the GPU may still be presenting the
		// previous frame's surface texture.  Flag the resize so BeginFrame()
		// handles it after the present has completed and the stale frame
		// texture/view have been released.
		m_needsResize = true;
	}

	void WebGPUGraphicsDevice::AddToDeletionQueue(std::function<void()> &&deleteFunc)
	{
		m_deletionQueue.push_back(std::move(deleteFunc));
	}

	void WebGPUGraphicsDevice::FlushDeletionQueue()
	{
		for (auto &func : m_deletionQueue)
		{
			func();
		}
		m_deletionQueue.clear();
	}

	void *WebGPUGraphicsDevice::GetNativeHandle() const
	{
		return static_cast<void *>(static_cast<WGPUDevice>(m_device));
	}

	wgpu::BufferUsage WebGPUGraphicsDevice::ConvertBufferUsage(EBufferUsage usage)
	{
		WGPUBufferUsage result = WGPUBufferUsage_None;

		if ((usage & EBufferUsage::Vertex) != EBufferUsage::None)
		{
			result |= WGPUBufferUsage_Vertex;
		}
		if ((usage & EBufferUsage::Index) != EBufferUsage::None)
		{
			result |= WGPUBufferUsage_Index;
		}
		if ((usage & EBufferUsage::Uniform) != EBufferUsage::None)
		{
			result |= WGPUBufferUsage_Uniform;
		}
		if ((usage & EBufferUsage::Storage) != EBufferUsage::None)
		{
			result |= WGPUBufferUsage_Storage;
		}
		if ((usage & EBufferUsage::CopySource) != EBufferUsage::None)
		{
			result |= WGPUBufferUsage_CopySrc;
		}
		if ((usage & EBufferUsage::CopyDestination) != EBufferUsage::None)
		{
			result |= WGPUBufferUsage_CopyDst;
		}
		if ((usage & EBufferUsage::Indirect) != EBufferUsage::None)
		{
			result |= WGPUBufferUsage_Indirect;
		}

		return static_cast<wgpu::BufferUsage>(result);
	}

	wgpu::TextureUsage WebGPUGraphicsDevice::ConvertTextureUsage(ETextureUsage usage)
	{
		WGPUTextureUsage result = WGPUTextureUsage_None;

		if (Bitwise::HasCompositeFlag(usage, ETextureUsage::Sampled))
		{
			result |= WGPUTextureUsage_TextureBinding;
		}
		if (Bitwise::HasCompositeFlag(usage, ETextureUsage::Storage))
		{
			result |= WGPUTextureUsage_StorageBinding;
		}
		if (Bitwise::HasCompositeFlag(usage, ETextureUsage::RenderTarget) ||
			Bitwise::HasCompositeFlag(usage, ETextureUsage::DepthStencil))
		{
			result |= WGPUTextureUsage_RenderAttachment;
		}
		if (Bitwise::HasCompositeFlag(usage, ETextureUsage::CopySource))
		{
			result |= WGPUTextureUsage_CopySrc;
		}
		if (Bitwise::HasCompositeFlag(usage, ETextureUsage::CopyDestination))
		{
			result |= WGPUTextureUsage_CopyDst;
		}

		return static_cast<wgpu::TextureUsage>(result);
	}

	wgpu::TextureFormat WebGPUGraphicsDevice::ConvertTextureFormat(ETextureFormat format)
	{
		switch (format)
		{
		case ETextureFormat::R8_UNORM:
			return wgpu::TextureFormat::R8Unorm;
		case ETextureFormat::R8_SNORM:
			return wgpu::TextureFormat::R8Snorm;
		case ETextureFormat::R8_UINT:
			return wgpu::TextureFormat::R8Uint;
		case ETextureFormat::R8_SINT:
			return wgpu::TextureFormat::R8Sint;

		case ETextureFormat::R16_UNORM:
			return wgpu::TextureFormat::R16Uint; // R16Unorm not in core WebGPU, use Uint
		case ETextureFormat::R16_SNORM:
			return wgpu::TextureFormat::R16Sint; // R16Snorm not in core WebGPU, use Sint
		case ETextureFormat::R16_UINT:
			return wgpu::TextureFormat::R16Uint;
		case ETextureFormat::R16_SINT:
			return wgpu::TextureFormat::R16Sint;
		case ETextureFormat::R16_FLOAT:
			return wgpu::TextureFormat::R16Float;

		case ETextureFormat::R32_UINT:
			return wgpu::TextureFormat::R32Uint;
		case ETextureFormat::R32_SINT:
			return wgpu::TextureFormat::R32Sint;
		case ETextureFormat::R32_FLOAT:
			return wgpu::TextureFormat::R32Float;

		case ETextureFormat::RG8_UNORM:
			return wgpu::TextureFormat::RG8Unorm;
		case ETextureFormat::RG8_SNORM:
			return wgpu::TextureFormat::RG8Snorm;
		case ETextureFormat::RG16_FLOAT:
			return wgpu::TextureFormat::RG16Float;
		case ETextureFormat::RG32_FLOAT:
			return wgpu::TextureFormat::RG32Float;

		case ETextureFormat::RGBA8_UNORM:
			return wgpu::TextureFormat::RGBA8Unorm;
		case ETextureFormat::RGBA8_SRGB:
			return wgpu::TextureFormat::RGBA8UnormSrgb;
		case ETextureFormat::RGBA16_FLOAT:
			return wgpu::TextureFormat::RGBA16Float;
		case ETextureFormat::RGBA32_FLOAT:
			return wgpu::TextureFormat::RGBA32Float;

		case ETextureFormat::BGRA8_UNORM:
			return wgpu::TextureFormat::BGRA8Unorm;
		case ETextureFormat::BGRA8_SRGB:
			return wgpu::TextureFormat::BGRA8UnormSrgb;

		case ETextureFormat::D16_UNORM:
			return wgpu::TextureFormat::Depth16Unorm;
		case ETextureFormat::D24_UNORM:
			return wgpu::TextureFormat::Depth24Plus;
		case ETextureFormat::D32_FLOAT:
			return wgpu::TextureFormat::Depth32Float;
		case ETextureFormat::D24_UNORM_S8_UINT:
			return wgpu::TextureFormat::Depth24PlusStencil8;
		case ETextureFormat::D32_FLOAT_S8_UINT:
			return wgpu::TextureFormat::Depth32FloatStencil8;

		case ETextureFormat::BC1_UNORM:
			return wgpu::TextureFormat::BC1RGBAUnorm;
		case ETextureFormat::BC1_SRGB:
			return wgpu::TextureFormat::BC1RGBAUnormSrgb;
		case ETextureFormat::BC3_UNORM:
			return wgpu::TextureFormat::BC3RGBAUnorm;
		case ETextureFormat::BC3_SRGB:
			return wgpu::TextureFormat::BC3RGBAUnormSrgb;
		case ETextureFormat::BC4_UNORM:
			return wgpu::TextureFormat::BC4RUnorm;
		case ETextureFormat::BC5_UNORM:
			return wgpu::TextureFormat::BC5RGUnorm;
		case ETextureFormat::BC7_UNORM:
			return wgpu::TextureFormat::BC7RGBAUnorm;
		case ETextureFormat::BC7_SRGB:
			return wgpu::TextureFormat::BC7RGBAUnormSrgb;

		default:
			LogWarn("Unknown texture format, defaulting to RGBA8Unorm");
			return wgpu::TextureFormat::RGBA8Unorm;
		}
	}

	ETextureFormat WebGPUGraphicsDevice::ConvertToEngineTextureFormat(wgpu::TextureFormat format)
	{
		switch (format)
		{
		case wgpu::TextureFormat::R8Unorm:
			return ETextureFormat::R8_UNORM;
		case wgpu::TextureFormat::R8Snorm:
			return ETextureFormat::R8_SNORM;
		case wgpu::TextureFormat::R8Uint:
			return ETextureFormat::R8_UINT;
		case wgpu::TextureFormat::R8Sint:
			return ETextureFormat::R8_SINT;

		case wgpu::TextureFormat::R16Uint:
			return ETextureFormat::R16_UNORM; // Assuming original was UNORM
		case wgpu::TextureFormat::R16Sint:
			return ETextureFormat::R16_SNORM; // Assuming original was SNORM
		case wgpu::TextureFormat::R16Float:
			return ETextureFormat::R16_FLOAT;

		case wgpu::TextureFormat::R32Uint:
			return ETextureFormat::R32_UINT;
		case wgpu::TextureFormat::R32Sint:
			return ETextureFormat::R32_SINT;
		case wgpu::TextureFormat::R32Float:
			return ETextureFormat::R32_FLOAT;

		case wgpu::TextureFormat::RG8Unorm:
			return ETextureFormat::RG8_UNORM;
		case wgpu::TextureFormat::RG8Snorm:
			return ETextureFormat::RG8_SNORM;
		case wgpu::TextureFormat::RG16Float:
			return ETextureFormat::RG16_FLOAT;
		case wgpu::TextureFormat::RG32Float:
			return ETextureFormat::RG32_FLOAT;

		case wgpu::TextureFormat::RGBA8Unorm:
			return ETextureFormat::RGBA8_UNORM;
		case wgpu::TextureFormat::RGBA8UnormSrgb:
			return ETextureFormat::RGBA8_SRGB;
		case wgpu::TextureFormat::RGBA16Float:
			return ETextureFormat::RGBA16_FLOAT;
		case wgpu::TextureFormat::RGBA32Float:
			return ETextureFormat::RGBA32_FLOAT;

		case wgpu::TextureFormat::BGRA8Unorm:
			return ETextureFormat::BGRA8_UNORM;
		case wgpu::TextureFormat::BGRA8UnormSrgb:
			return ETextureFormat::BGRA8_SRGB;

		default:
			LogWarn("Unknown WebGPU texture format, defaulting to RGBA8_UNORM");
			return ETextureFormat::RGBA8_UNORM;
		}
	}

	void WebGPUGraphicsDevice::OnDeviceError(WGPUErrorType type, char const *message, void * /*userdata*/)
	{
		LogFormat(ELogLevel::Error, "WebGPU Error ({}): {}", magic_enum::enum_name(type),
				  message != nullptr ? message : "Unknown error");
	}

	void WebGPUGraphicsDevice::OnDeviceLost(WGPUDeviceLostReason reason, char const *message, void *userdata)
	{
		auto *device = static_cast<WebGPUGraphicsDevice *>(userdata);

		LogFormat(ELogLevel::Error, "WebGPU Device Lost ({}): {}", magic_enum::enum_name(reason),
				  message != nullptr ? message : "Unknown reason");

		if (device != nullptr)
		{
			device->m_initialized = false;
		}
	}

	void WebGPUGraphicsDevice::PollEvents()
	{
#ifdef WEBGPU_BACKEND_WGPU
		m_device.poll(true, nullptr);
#endif
	}

	ETextureFormat WebGPUGraphicsDevice::GetPreferredSwapchainFormat() const
	{
		return ConvertToEngineTextureFormat(m_surfaceFormat);
	}

} // namespace Hush::Graphics
