/*! \file GraphicsTypes.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Common types, enumerations, and structures for graphics abstraction
*/
#pragma once

#include <cstdint>
#include <vector>

namespace Hush::Graphics
{
	/// @brief Graphics API backend type
	enum class EGraphicsAPI
	{
		Vulkan,
		D3D12,
		Metal,
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

	/// @brief Return the number of bytes per pixel for uncompressed formats.
	///        Compressed / depth-stencil formats return a best-effort value
	///        (block size divided by texel count is NOT attempted — callers
	///        dealing with BC formats should use dedicated block-size helpers).
	constexpr uint32_t GetBytesPerPixel(ETextureFormat format)
	{
		switch (format)
		{
		// 1 byte
		case ETextureFormat::R8_UNORM:
		case ETextureFormat::R8_SNORM:
		case ETextureFormat::R8_UINT:
		case ETextureFormat::R8_SINT:
			return 1;

		// 2 bytes
		case ETextureFormat::R16_UNORM:
		case ETextureFormat::R16_SNORM:
		case ETextureFormat::R16_UINT:
		case ETextureFormat::R16_SINT:
		case ETextureFormat::R16_FLOAT:
		case ETextureFormat::RG8_UNORM:
		case ETextureFormat::RG8_SNORM:
		case ETextureFormat::D16_UNORM:
			return 2;

		// 3 bytes
		case ETextureFormat::RGB8_UNORM:
		case ETextureFormat::RGB8_SRGB:
			return 3;

		// 4 bytes
		case ETextureFormat::R32_UINT:
		case ETextureFormat::R32_SINT:
		case ETextureFormat::R32_FLOAT:
		case ETextureFormat::RG16_FLOAT:
		case ETextureFormat::RGBA8_UNORM:
		case ETextureFormat::RGBA8_SRGB:
		case ETextureFormat::BGRA8_UNORM:
		case ETextureFormat::BGRA8_SRGB:
		case ETextureFormat::D24_UNORM:
		case ETextureFormat::D32_FLOAT:
		case ETextureFormat::D24_UNORM_S8_UINT:
			return 4;

		// 8 bytes
		case ETextureFormat::RG32_FLOAT:
		case ETextureFormat::RGBA16_FLOAT:
		case ETextureFormat::D32_FLOAT_S8_UINT:
			return 8;

		// 16 bytes
		case ETextureFormat::RGBA32_FLOAT:
			return 16;

		// Block-compressed formats — return the block size in bytes.
		// Callers must account for 4x4 block granularity themselves.
		case ETextureFormat::BC1_UNORM:
		case ETextureFormat::BC1_SRGB:
		case ETextureFormat::BC4_UNORM:
			return 8; // 8 bytes per 4x4 block

		case ETextureFormat::BC3_UNORM:
		case ETextureFormat::BC3_SRGB:
		case ETextureFormat::BC5_UNORM:
		case ETextureFormat::BC7_UNORM:
		case ETextureFormat::BC7_SRGB:
			return 16; // 16 bytes per 4x4 block

		default:
			return 4;
		}
	}

	/// @brief Memory access flags
	enum class EMemoryAccess
	{
		CPUNone = 0,					   // GPU only
		CPUWrite = 1 << 0,				   // CPU can write, GPU can read
		CPURead = 1 << 1,				   // CPU can read, GPU can write
		CPUReadWrite = CPUWrite | CPURead, // CPU can read/write
	};

	inline EMemoryAccess operator|(EMemoryAccess a, EMemoryAccess b)
	{
		return static_cast<EMemoryAccess>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EMemoryAccess operator&(EMemoryAccess a, EMemoryAccess b)
	{
		return static_cast<EMemoryAccess>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	inline EMemoryAccess &operator|=(EMemoryAccess &a, EMemoryAccess b)
	{
		a = a | b;
		return a;
	}

	inline bool HasFlag(EMemoryAccess access, EMemoryAccess flag)
	{
		return (static_cast<uint32_t>(access) & static_cast<uint32_t>(flag)) != 0;
	}

	/// @brief Comparison function used for depth/stencil tests and comparison samplers.
	///
	/// When used with a sampler, Undefined means "no comparison" (regular filtering sampler).
	/// For depth/stencil state, use one of the concrete comparison functions (Never..Always).
	enum class ECompareFunction : uint32_t
	{
		/// @brief No comparison — used for regular filtering samplers.
		Undefined = 0,

		/// @brief Comparison never passes.
		Never,

		/// @brief Passes if reference < fetched value.
		Less,

		/// @brief Passes if reference == fetched value.
		Equal,

		/// @brief Passes if reference <= fetched value.
		LessEqual,

		/// @brief Passes if reference > fetched value.
		Greater,

		/// @brief Passes if reference != fetched value.
		NotEqual,

		/// @brief Passes if reference >= fetched value.
		GreaterEqual,

		/// @brief Comparison always passes.
		Always,
	};

	/// @brief Queue type for command submission
	enum class EQueueType
	{
		Graphics,
		Compute,
		Transfer,
	};

	/// @brief Resource state flags for barrier transitions.
	///
	/// These represent the logical usage state of a GPU resource.
	/// A resource can be in a combined read state (multiple read flags OR'd together),
	/// but only one write state at a time.
	///
	/// Modeled after D3D12 resource states / Vulkan image layouts.
	enum class EResourceState : uint32_t
	{
		Undefined = 0,

		// Read states (can be combined)
		VertexBuffer = 1 << 0,
		IndexBuffer = 1 << 1,
		ConstantBuffer = 1 << 2,
		NonPixelShaderAccess = 1 << 3,
		PixelShaderAccess = 1 << 4,
		IndirectArgument = 1 << 5,
		CopySource = 1 << 6,
		DepthStencilRead = 1 << 7,
		Present = 1 << 8,

		// Write states (mutually exclusive with each other)
		RenderTarget = 1 << 16,
		DepthStencilWrite = 1 << 17,
		UnorderedAccess = 1 << 18,
		CopyDestination = 1 << 19,

		// Convenience combinations
		AnyShaderAccess = NonPixelShaderAccess | PixelShaderAccess,
		GenericRead = VertexBuffer | IndexBuffer | ConstantBuffer | AnyShaderAccess | IndirectArgument | CopySource,
	};

	inline EResourceState operator|(EResourceState a, EResourceState b)
	{
		return static_cast<EResourceState>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EResourceState operator&(EResourceState a, EResourceState b)
	{
		return static_cast<EResourceState>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	inline EResourceState &operator|=(EResourceState &a, EResourceState b)
	{
		a = a | b;
		return a;
	}

	inline bool HasFlag(EResourceState state, EResourceState flag)
	{
		return (static_cast<uint32_t>(state) & static_cast<uint32_t>(flag)) != 0;
	}

	/// @brief Returns true if the state contains any write flag
	inline bool IsWriteState(EResourceState state)
	{
		constexpr uint32_t writeMask = static_cast<uint32_t>(EResourceState::RenderTarget) |
									   static_cast<uint32_t>(EResourceState::DepthStencilWrite) |
									   static_cast<uint32_t>(EResourceState::UnorderedAccess) |
									   static_cast<uint32_t>(EResourceState::CopyDestination);
		return (static_cast<uint32_t>(state) & writeMask) != 0;
	}

	/// @brief Describes a resource state transition barrier
	struct ResourceBarrierDescriptor
	{
		/// Opaque pointer to the resource (IGraphicsBuffer* or IGraphicsTexture*)
		void *resource = nullptr;
		/// State before the barrier
		EResourceState stateBefore = EResourceState::Undefined;
		/// State after the barrier
		EResourceState stateAfter = EResourceState::Undefined;
		/// Subresource index (use UINT32_MAX for all subresources)
		uint32_t subresource = UINT32_MAX;
	};

	/// @brief Describes a split barrier (begin/end pair for overlapped transitions).
	///
	/// Split barriers allow the GPU to begin a transition early and complete it later,
	/// hiding latency by overlapping the transition with other work.
	struct SplitBarrierDescriptor
	{
		void *resource = nullptr;
		EResourceState stateBefore = EResourceState::Undefined;
		EResourceState stateAfter = EResourceState::Undefined;
		uint32_t subresource = UINT32_MAX;
	};

	/// @brief UAV (Unordered Access View) barrier — ensures all UAV writes complete
	/// before subsequent UAV reads or writes on the same resource.
	struct UAVBarrierDescriptor
	{
		void *resource = nullptr; ///< nullptr means barrier on all UAV resources
	};

	/// @brief A batch of barriers to issue in a single call
	struct BarrierGroup
	{
		std::vector<ResourceBarrierDescriptor> transitions;
		std::vector<UAVBarrierDescriptor> uavBarriers;
		std::vector<SplitBarrierDescriptor> beginSplitBarriers;
		std::vector<SplitBarrierDescriptor> endSplitBarriers;
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

		/// @brief Whether the device exposes a dedicated async compute queue
		bool hasAsyncComputeQueue = false;
		/// @brief Whether the device exposes a dedicated transfer/copy queue
		bool hasDedicatedTransferQueue = false;
		/// @brief Whether the device supports timeline (monotonic) fences
		bool supportsTimelineFences = false;

		/// @brief Bitmask of EResourceState values supported for transitions on the graphics queue.
		/// Graphics queues typically support all states.
		uint32_t graphicsQueueSupportedStates = static_cast<uint32_t>(EResourceState::GenericRead) |
												static_cast<uint32_t>(EResourceState::RenderTarget) |
												static_cast<uint32_t>(EResourceState::DepthStencilWrite) |
												static_cast<uint32_t>(EResourceState::UnorderedAccess) |
												static_cast<uint32_t>(EResourceState::CopyDestination);

		/// @brief Bitmask of EResourceState values supported for transitions on the compute queue.
		/// Compute queues cannot transition pixel-shader-related or render-target states.
		uint32_t computeQueueSupportedStates = static_cast<uint32_t>(EResourceState::NonPixelShaderAccess) |
											   static_cast<uint32_t>(EResourceState::UnorderedAccess) |
											   static_cast<uint32_t>(EResourceState::CopySource) |
											   static_cast<uint32_t>(EResourceState::CopyDestination);

		/// @brief Bitmask of EResourceState values supported for transitions on the transfer queue.
		/// Transfer queues can only handle copy states.
		uint32_t transferQueueSupportedStates =
			static_cast<uint32_t>(EResourceState::CopySource) | static_cast<uint32_t>(EResourceState::CopyDestination);
	};
} // namespace Hush::Graphics
