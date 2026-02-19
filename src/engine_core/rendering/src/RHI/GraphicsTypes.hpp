/*! \file GraphicsTypes.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Common types, enumerations, and structures for graphics abstraction
*/
#pragma once

#include <cstdint>

namespace Hush::Graphics
{
	/// @brief Graphics API backend type
	enum class EGraphicsAPI
	{
		Vulkan,
		D3D12,
		Metal,
		OpenGL,
		WebGPU,
	};

	/// @brief Buffer usage flags
	enum class EBufferUsage : uint32_t
	{
		None = 0,
		Vertex = 1 << 0,
		Index = 1 << 1,
		Uniform = 1 << 2,
		Storage = 1 << 3,
		CopySource = 1 << 4,
		CopyDestination = 1 << 5,
		Indirect = 1 << 6,
	};

	inline EBufferUsage operator|(EBufferUsage a, EBufferUsage b)
	{
		return static_cast<EBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EBufferUsage operator&(EBufferUsage a, EBufferUsage b)
	{
		return static_cast<EBufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	/// @brief Texture usage flags
	enum class ETextureUsage : uint32_t
	{
		None = 0,
		Sampled = 1 << 0,
		Storage = 1 << 1,
		RenderTarget = 1 << 2,
		DepthStencil = 1 << 3,
		CopySource = 1 << 4,
		CopyDestination = 1 << 5,
	};

	inline ETextureUsage operator|(ETextureUsage a, ETextureUsage b)
	{
		return static_cast<ETextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline ETextureUsage operator&(ETextureUsage a, ETextureUsage b)
	{
		return static_cast<ETextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	// NOLINTBEGIN(readability-identifier-naming)
	/// @brief Texture format
	enum class ETextureFormat
	{
		// 8-bit formats
		R8_UNORM,
		R8_SNORM,
		R8_UINT,
		R8_SINT,

		// 16-bit formats
		R16_UNORM,
		R16_SNORM,
		R16_UINT,
		R16_SINT,
		R16_FLOAT,

		// 32-bit formats
		R32_UINT,
		R32_SINT,
		R32_FLOAT,

		// RG formats
		RG8_UNORM,
		RG8_SNORM,
		RG16_FLOAT,
		RG32_FLOAT,

		// RGB formats
		RGB8_UNORM,
		RGB8_SRGB,

		// RGBA formats
		RGBA8_UNORM,
		RGBA8_SRGB,
		RGBA16_FLOAT,
		RGBA32_FLOAT,

		// BGRA formats
		BGRA8_UNORM,
		BGRA8_SRGB,

		// Depth formats
		D16_UNORM,
		D24_UNORM,
		D32_FLOAT,
		D24_UNORM_S8_UINT,
		D32_FLOAT_S8_UINT,

		// Compressed formats
		BC1_UNORM,
		BC1_SRGB,
		BC3_UNORM,
		BC3_SRGB,
		BC4_UNORM,
		BC5_UNORM,
		BC7_UNORM,
		BC7_SRGB,
	};
	// NOLINTEND(readability-identifier-naming)

	/// @brief Memory access flags
	enum class EMemoryAccess
	{
		CPUNone,	  // GPU only
		CPUWrite,	  // CPU can write, GPU can read
		CPURead,	  // CPU can read, GPU can write
		CPUReadWrite, // CPU can read/write
	};

	/// @brief Queue type for command submission
	enum class EQueueType
	{
		Graphics,
		Compute,
		Transfer,
	};

	/// @brief Buffer creation descriptor
	struct BufferDescriptor
	{
		uint64_t size = 0;
		EBufferUsage usage = EBufferUsage::None;
		EMemoryAccess memoryAccess = EMemoryAccess::CPUNone;
		const char *debugName = nullptr;
	};

	/// @brief Texture creation descriptor
	struct TextureDescriptor
	{
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		uint32_t mipLevels = 1;
		uint32_t arrayLayers = 1;
		uint32_t sampleCount = 1;
		ETextureFormat format = ETextureFormat::RGBA8_UNORM;
		ETextureUsage usage = ETextureUsage::Sampled;
		const char *debugName = nullptr;

		bool ownedByExternalSource =
			false; // Indicates if the texture is managed externally (e.g., swapchain backbuffer)
	};

	/// @brief Device capabilities
	struct GraphicsDeviceCapabilities
	{
		uint64_t maxBufferSize = 0;
		uint32_t maxTextureDimension2D = 0;
		uint32_t maxTextureDimension3D = 0;
		uint32_t maxTextureArrayLayers = 0;
		uint64_t maxUniformBufferBindingSize = 0;
		uint64_t maxStorageBufferBindingSize = 0;
		uint32_t maxColorAttachments = 0;
		bool supportsCompute = false;
		bool supportsGeometryShader = false;
		bool supportsTessellation = false;
		bool supportsRayTracing = false;
	};

} // namespace Hush::Graphics
