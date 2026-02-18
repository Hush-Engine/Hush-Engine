/*! \file WebGPUGraphicsDevice.cpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU implementation of IGraphicsDevice interface
*/
#include "WebGPUGraphicsDevice.hpp"
#include "Logger.hpp"
#include "Assertions.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <sdl2webgpu/sdl2webgpu.h>
#include <magic_enum/magic_enum.hpp>

namespace Hush::Graphics
{

// ============================================================================
// WebGPUBuffer Implementation
// ============================================================================

WebGPUBuffer::WebGPUBuffer(wgpu::Buffer buffer, const BufferDescriptor& desc)
	: m_buffer(buffer)
	, m_descriptor(desc)
{
}

WebGPUBuffer::~WebGPUBuffer()
{
	if (m_mappedData != nullptr)
	{
		Unmap();
	}
	m_buffer.destroy();
}

void* WebGPUBuffer::Map()
{
	if (m_mappedData != nullptr)
	{
		return m_mappedData;
	}

	// Note: WebGPU mapping is async, simplified here for basic usage
	// In production, you'd use mapAsync with callbacks
	LogWarn("WebGPU buffer mapping not fully implemented");
	return nullptr;
}

void WebGPUBuffer::Unmap()
{
	if (m_mappedData == nullptr)
	{
		return;
	}

	m_buffer.unmap();
	m_mappedData = nullptr;
}

void* WebGPUBuffer::GetNativeHandle() const
{
	return static_cast<void*>(static_cast<WGPUBuffer>(m_buffer));
}

// ============================================================================
// WebGPUTexture Implementation
// ============================================================================

WebGPUTexture::WebGPUTexture(wgpu::Texture texture, wgpu::TextureView view, const TextureDescriptor& desc)
	: m_texture(texture)
	, m_view(view)
	, m_descriptor(desc)
{
}

WebGPUTexture::~WebGPUTexture()
{
	m_view = nullptr;
	m_texture.destroy();
}

void* WebGPUTexture::GetNativeHandle() const
{
	return static_cast<void*>(static_cast<WGPUTexture>(m_texture));
}

// ============================================================================
// WebGPUCommandQueue Implementation
// ============================================================================

WebGPUCommandQueue::WebGPUCommandQueue(wgpu::Queue queue, EQueueType type)
	: m_queue(queue)
	, m_queueType(type)
{
}

void WebGPUCommandQueue::Submit(void* commands)
{
	if (commands == nullptr)
	{
		return;
	}

	auto* cmdBuffer = static_cast<wgpu::CommandBuffer*>(commands);
	m_queue.submit(1, cmdBuffer);
}

void WebGPUCommandQueue::WaitIdle()
{
	// WebGPU doesn't have direct waitIdle
	// Could implement with fences/callbacks if needed
}

void* WebGPUCommandQueue::GetNativeHandle() const
{
	return static_cast<void*>(static_cast<WGPUQueue>(m_queue));
}

wgpu::CommandEncoder WebGPUCommandQueue::CreateEncoder(const char* /*label*/)
{
	// Note: This helper method is not used in the current implementation
	// Command encoders should be created directly from the device
	return {};
}

// ============================================================================
// WebGPUGraphicsDevice Implementation
// ============================================================================

WebGPUGraphicsDevice::WebGPUGraphicsDevice(void* windowHandle)
	: m_windowHandle(windowHandle)

{
	LogTrace("Initializing WebGPU Graphics Device");

	InitializeInstance();
	InitializeAdapter();
	InitializeDevice();
	CreateSurface();

	// Get initial window size
	auto* window = static_cast<SDL_Window*>(m_windowHandle);
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

	// TODO: Initialize device properly
	// For now, using nullptr to isolate compilation issues

	LogTrace("WebGPU device initialization skipped (stub)");
}

void WebGPUGraphicsDevice::CreateSurface()
{
	auto* window = static_cast<SDL_Window*>(m_windowHandle);
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
	config.usage = wgpu::TextureUsage::RenderAttachment;
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
	m_capabilities.supportsCompute = true; // WebGPU always supports compute
	m_capabilities.supportsGeometryShader = false; // Not in WebGPU
	m_capabilities.supportsTessellation = false; // Not in WebGPU
	m_capabilities.supportsRayTracing = false; // Not in WebGPU
}

// ============================================================================
// IGraphicsDevice Implementation
// ============================================================================

Hush::Graphics::GraphicsDeviceCapabilities WebGPUGraphicsDevice::GetCapabilities() const
{
	return m_capabilities;
}

std::shared_ptr<IGraphicsBuffer> WebGPUGraphicsDevice::CreateBuffer(const BufferDescriptor& descriptor)
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

std::shared_ptr<IGraphicsTexture> WebGPUGraphicsDevice::CreateTexture(const TextureDescriptor& descriptor)
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

	// TODO: Create texture properly once device initialization is fixed
	LogError("Texture creation not yet implemented");
	return nullptr;
}

void WebGPUGraphicsDevice::WriteBuffer(IGraphicsBuffer* buffer, const void* data, uint64_t size, uint64_t offset)
{
	if (buffer == nullptr || data == nullptr || size == 0)
	{
		return;
	}

	auto* wgpuBuffer = dynamic_cast<WebGPUBuffer*>(buffer);
	wgpu::Queue queue = m_device.getQueue();
	queue.writeBuffer(wgpuBuffer->GetBuffer(), offset, data, size);
}

void WebGPUGraphicsDevice::WriteTexture(IGraphicsTexture* texture, const void* data, uint32_t width, uint32_t height, uint32_t mipLevel)
{
	if (texture == nullptr || data == nullptr)
	{
		return;
	}

	auto* wgpuTexture = dynamic_cast<WebGPUTexture*>(texture);

	wgpu::TexelCopyTextureInfo destination{};
	destination.texture = wgpuTexture->GetTexture();
	destination.mipLevel = mipLevel;
	destination.origin.x = 0;
	destination.origin.y = 0;
	destination.origin.z = 0;
	destination.aspect = wgpu::TextureAspect::All;

	wgpu::TexelCopyBufferLayout dataLayout{};
	dataLayout.offset = 0;
	dataLayout.bytesPerRow = width * 4; // Assuming RGBA8
	dataLayout.rowsPerImage = height;

	wgpu::Extent3D writeSize{};
	writeSize.width = width;
	writeSize.height = height;
	writeSize.depthOrArrayLayers = 1;

	wgpu::Queue queue = m_device.getQueue();
	queue.writeTexture(destination, data, static_cast<size_t>(width) * height * 4, dataLayout, writeSize);
}

void* WebGPUGraphicsDevice::BeginFrame()
{
	if (m_needsResize)
	{
		auto* window = static_cast<SDL_Window*>(m_windowHandle);
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

		return nullptr;
	}

	wgpu::Texture texture = surfaceTexture.texture;
	m_currentFrameView = texture.createView();
	return static_cast<void*>(static_cast<WGPUTextureView>(m_currentFrameView));
}

void WebGPUGraphicsDevice::EndFrame()
{
	m_surface.present();
	m_currentFrameView = nullptr;
	FlushDeletionQueue();
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

void WebGPUGraphicsDevice::AddToDeletionQueue(std::function<void()>&& deleteFunc)
{
	m_deletionQueue.push_back(std::move(deleteFunc));
}

void WebGPUGraphicsDevice::FlushDeletionQueue()
{
	for (auto& func : m_deletionQueue)
	{
		func();
	}
	m_deletionQueue.clear();
}

void* WebGPUGraphicsDevice::GetNativeHandle() const
{
	return static_cast<void*>(static_cast<WGPUDevice>(m_device));
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
		case ETextureFormat::R8_UNORM: return wgpu::TextureFormat::R8Unorm;
		case ETextureFormat::R8_SNORM: return wgpu::TextureFormat::R8Snorm;
		case ETextureFormat::R8_UINT: return wgpu::TextureFormat::R8Uint;
		case ETextureFormat::R8_SINT: return wgpu::TextureFormat::R8Sint;

		case ETextureFormat::R16_UNORM: return wgpu::TextureFormat::R16Uint; // R16Unorm not in core WebGPU, use Uint
		case ETextureFormat::R16_SNORM: return wgpu::TextureFormat::R16Sint; // R16Snorm not in core WebGPU, use Sint
		case ETextureFormat::R16_UINT: return wgpu::TextureFormat::R16Uint;
		case ETextureFormat::R16_SINT: return wgpu::TextureFormat::R16Sint;
		case ETextureFormat::R16_FLOAT: return wgpu::TextureFormat::R16Float;

		case ETextureFormat::R32_UINT: return wgpu::TextureFormat::R32Uint;
		case ETextureFormat::R32_SINT: return wgpu::TextureFormat::R32Sint;
		case ETextureFormat::R32_FLOAT: return wgpu::TextureFormat::R32Float;

		case ETextureFormat::RG8_UNORM: return wgpu::TextureFormat::RG8Unorm;
		case ETextureFormat::RG8_SNORM: return wgpu::TextureFormat::RG8Snorm;
		case ETextureFormat::RG16_FLOAT: return wgpu::TextureFormat::RG16Float;
		case ETextureFormat::RG32_FLOAT: return wgpu::TextureFormat::RG32Float;

		case ETextureFormat::RGBA8_UNORM: return wgpu::TextureFormat::RGBA8Unorm;
		case ETextureFormat::RGBA8_SRGB: return wgpu::TextureFormat::RGBA8UnormSrgb;
		case ETextureFormat::RGBA16_FLOAT: return wgpu::TextureFormat::RGBA16Float;
		case ETextureFormat::RGBA32_FLOAT: return wgpu::TextureFormat::RGBA32Float;

		case ETextureFormat::BGRA8_UNORM: return wgpu::TextureFormat::BGRA8Unorm;
		case ETextureFormat::BGRA8_SRGB: return wgpu::TextureFormat::BGRA8UnormSrgb;

		case ETextureFormat::D16_UNORM: return wgpu::TextureFormat::Depth16Unorm;
		case ETextureFormat::D24_UNORM: return wgpu::TextureFormat::Depth24Plus;
		case ETextureFormat::D32_FLOAT: return wgpu::TextureFormat::Depth32Float;
		case ETextureFormat::D24_UNORM_S8_UINT: return wgpu::TextureFormat::Depth24PlusStencil8;
		case ETextureFormat::D32_FLOAT_S8_UINT: return wgpu::TextureFormat::Depth32FloatStencil8;

		case ETextureFormat::BC1_UNORM: return wgpu::TextureFormat::BC1RGBAUnorm;
		case ETextureFormat::BC1_SRGB: return wgpu::TextureFormat::BC1RGBAUnormSrgb;
		case ETextureFormat::BC3_UNORM: return wgpu::TextureFormat::BC3RGBAUnorm;
		case ETextureFormat::BC3_SRGB: return wgpu::TextureFormat::BC3RGBAUnormSrgb;
		case ETextureFormat::BC4_UNORM: return wgpu::TextureFormat::BC4RUnorm;
		case ETextureFormat::BC5_UNORM: return wgpu::TextureFormat::BC5RGUnorm;
		case ETextureFormat::BC7_UNORM: return wgpu::TextureFormat::BC7RGBAUnorm;
		case ETextureFormat::BC7_SRGB: return wgpu::TextureFormat::BC7RGBAUnormSrgb;

		default:
			LogWarn("Unknown texture format, defaulting to RGBA8Unorm");
			return wgpu::TextureFormat::RGBA8Unorm;
	}
}

// ============================================================================
// Callbacks
// ============================================================================

void WebGPUGraphicsDevice::OnDeviceError(WGPUErrorType type, char const* message, void* /*userdata*/)
{
	LogFormat(ELogLevel::Error, "WebGPU Error ({}): {}",
		magic_enum::enum_name(type), message != nullptr ? message : "Unknown error");
}

void WebGPUGraphicsDevice::OnDeviceLost(WGPUDeviceLostReason reason, char const* message, void* userdata)
{
	auto* device = static_cast<WebGPUGraphicsDevice*>(userdata);

	LogFormat(ELogLevel::Error, "WebGPU Device Lost ({}): {}",
		magic_enum::enum_name(reason), message != nullptr ? message : "Unknown reason");

	if (device != nullptr)
	{
		device->m_initialized = false;
	}
}

} // namespace Hush::Graphics
