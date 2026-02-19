/*! \file WebGPUGraphicsDevice.cpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU implementation of IGraphicsDevice interface
*/
#include "WebGPUGraphicsDevice.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "RHI/IGraphicsTexture.hpp"
#include "WebGPU/WebGPUTexture.hpp"
#include "WebGPUCommandList.hpp"
#include "Logger.hpp"
#include "Assertions.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <sdl2webgpu/sdl2webgpu.h>
#include <magic_enum/magic_enum.hpp>
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{

	// ============================================================================
	// WebGPUGraphicsDevice Implementation
	// ============================================================================

	WebGPUGraphicsDevice::WebGPUGraphicsDevice(void *windowHandle)
		: m_windowHandle(windowHandle)
	{
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

	// ============================================================================
	// Initialization
	// ============================================================================

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
		LogFormat(ELogLevel::Info, "Surface configured: {}x{}", width, height);
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
	}

	// ============================================================================
	// IGraphicsDevice Implementation
	// ============================================================================

	Hush::Graphics::GraphicsDeviceCapabilities WebGPUGraphicsDevice::GetCapabilities() const
	{
		return m_capabilities;
	}

	std::shared_ptr<IGraphicsBuffer> WebGPUGraphicsDevice::CreateBuffer(const BufferDescriptor &descriptor)
	{
		wgpu::BufferDescriptor desc{};
		desc.size = descriptor.size;
		desc.usage = ConvertBufferUsage(descriptor.usage);
		desc.mappedAtCreation = static_cast<WGPUBool>(false);

		if (descriptor.debugName != nullptr)
		{
			desc.label = wgpu::StringView(descriptor.debugName);
		}

		// TODO: Create buffer properly once device initialization is fixed
		LogError("Buffer creation not yet implemented");
		return nullptr;
	}

	std::unique_ptr<IGraphicsTexture> WebGPUGraphicsDevice::CreateTexture(const TextureDescriptor &descriptor)
	{
		wgpu::TextureDescriptor desc{};
		desc.size.width = descriptor.width;
		desc.size.height = descriptor.height;
		desc.size.depthOrArrayLayers = descriptor.depth;
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
			viewDesc.dimension = wgpu::TextureViewDimension::_2D;
			viewDesc.baseMipLevel = 0;
			viewDesc.mipLevelCount = descriptor.mipLevels;
			viewDesc.baseArrayLayer = 0;
			viewDesc.arrayLayerCount = descriptor.depth;

			wgpu::TextureView textureView = texture.createView(viewDesc);

			return std::make_unique<Hush::Graphics::WebGPUTexture>(texture, textureView, descriptor);
		}

		return nullptr;
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

	void WebGPUGraphicsDevice::BeginFrame()
	{
		if (m_needsResize)
		{
			auto *window = static_cast<SDL_Window *>(m_windowHandle);
			int width = 0;
			int height = 0;
			SDL_GetWindowSize(window, &width, &height);
			if (width > 0 && height > 0)
			{
				Resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
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
												  .ownedByExternalSource = true,
											  });
	}

	void WebGPUGraphicsDevice::EndFrame()
	{
		m_surface.present();
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

		LogFormat(ELogLevel::Info, "Resizing device: {}x{} -> {}x{}", m_width, m_height, width, height);
		ConfigureSurface(width, height);
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

	// ============================================================================
	// Conversion Helpers
	// ============================================================================

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

		if ((usage & ETextureUsage::Sampled) != ETextureUsage::None)
		{
			result |= WGPUTextureUsage_TextureBinding;
		}
		if ((usage & ETextureUsage::Storage) != ETextureUsage::None)
		{
			result |= WGPUTextureUsage_StorageBinding;
		}
		if ((usage & ETextureUsage::RenderTarget) != ETextureUsage::None)
		{
			result |= WGPUTextureUsage_RenderAttachment;
		}
		if ((usage & ETextureUsage::CopySource) != ETextureUsage::None)
		{
			result |= WGPUTextureUsage_CopySrc;
		}
		if ((usage & ETextureUsage::CopyDestination) != ETextureUsage::None)
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

	// ============================================================================
	// Callbacks
	// ============================================================================

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

} // namespace Hush::Graphics
