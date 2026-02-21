/*! \file PipelineDescriptor.hpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief Pipeline state descriptors for graphics and compute pipelines.
*/
#pragma once

#include "GraphicsTypes.hpp"
#include "IShaderModule.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace Hush::Graphics
{
	// Forward declarations
	class IShaderModule;
	class IBindGroupLayout;

	/// @brief Vertex attribute format (matches common GPU formats)
	enum class EVertexFormat : uint32_t
	{
		Float32,
		Float32x2,
		Float32x3,
		Float32x4,

		Sint32,
		Sint32x2,
		Sint32x3,
		Sint32x4,

		Uint32,
		Uint32x2,
		Uint32x3,
		Uint32x4,

		Float16x2,
		Float16x4,

		Uint8x2,
		Uint8x4,

		Sint8x2,
		Sint8x4,

		Unorm8x2,
		Unorm8x4,

		Snorm8x2,
		Snorm8x4,

		Uint16x2,
		Uint16x4,

		Sint16x2,
		Sint16x4,

		Unorm16x2,
		Unorm16x4,

		Snorm16x2,
		Snorm16x4,
	};

	/// @brief Returns the byte size of a single vertex attribute element.
	inline uint32_t GetVertexFormatSize(EVertexFormat format)
	{
		switch (format)
		{
		case EVertexFormat::Float32:
			return 4;
		case EVertexFormat::Float32x2:
			return 8;
		case EVertexFormat::Float32x3:
			return 12;
		case EVertexFormat::Float32x4:
			return 16;

		case EVertexFormat::Sint32:
		case EVertexFormat::Uint32:
			return 4;
		case EVertexFormat::Sint32x2:
		case EVertexFormat::Uint32x2:
			return 8;
		case EVertexFormat::Sint32x3:
		case EVertexFormat::Uint32x3:
			return 12;
		case EVertexFormat::Sint32x4:
		case EVertexFormat::Uint32x4:
			return 16;

		case EVertexFormat::Float16x2:
			return 4;
		case EVertexFormat::Float16x4:
			return 8;

		case EVertexFormat::Uint8x2:
		case EVertexFormat::Sint8x2:
		case EVertexFormat::Unorm8x2:
		case EVertexFormat::Snorm8x2:
			return 2;
		case EVertexFormat::Uint8x4:
		case EVertexFormat::Sint8x4:
		case EVertexFormat::Unorm8x4:
		case EVertexFormat::Snorm8x4:
			return 4;

		case EVertexFormat::Uint16x2:
		case EVertexFormat::Sint16x2:
		case EVertexFormat::Unorm16x2:
		case EVertexFormat::Snorm16x2:
			return 4;
		case EVertexFormat::Uint16x4:
		case EVertexFormat::Sint16x4:
		case EVertexFormat::Unorm16x4:
		case EVertexFormat::Snorm16x4:
			return 8;

		default:
			return 0;
		}
	}

	/// @brief Vertex input stepping mode
	enum class EVertexStepMode : uint32_t
	{
		Vertex = 0,	  ///< Attribute advances per vertex
		Instance = 1, ///< Attribute advances per instance
	};

	/// @brief Describes a single vertex attribute within a vertex buffer layout.
	struct VertexAttribute
	{
		/// @brief Shader location index (layout(location = N))
		uint32_t shaderLocation = 0;

		/// @brief Byte offset of this attribute within the vertex buffer stride
		uint32_t offset = 0;

		/// @brief Data format of the attribute
		EVertexFormat format = EVertexFormat::Float32x4;
	};

	/// @brief Describes the layout of a single vertex buffer binding.
	struct VertexBufferLayout
	{
		/// @brief Byte stride between consecutive elements
		uint32_t stride = 0;

		/// @brief Per-vertex or per-instance stepping
		EVertexStepMode stepMode = EVertexStepMode::Vertex;

		/// @brief Attributes sourced from this buffer
		std::vector<VertexAttribute> attributes;
	};

	/// @brief Primitive topology for the input assembler stage
	enum class EPrimitiveTopology : uint32_t
	{
		PointList = 0,
		LineList,
		LineStrip,
		TriangleList,
		TriangleStrip,
	};

	/// @brief Front face winding order
	enum class EFrontFace : uint32_t
	{
		CounterClockwise = 0,
		Clockwise,
	};

	/// @brief Cull mode for the rasterizer
	enum class ECullModeFlags : uint32_t
	{
		None = 0,
		Front,
		Back,
	};

	/// @brief Index format for indexed draw calls
	enum class EIndexFormat : uint32_t
	{
		Undefined = 0,
		Uint16,
		Uint32,
	};

	/// @brief Primitive state configuration
	struct PrimitiveState
	{
		EPrimitiveTopology topology = EPrimitiveTopology::TriangleList;
		EIndexFormat stripIndexFormat = EIndexFormat::Undefined; ///< Only for strip topologies
		EFrontFace frontFace = EFrontFace::CounterClockwise;
		ECullModeFlags cullMode = ECullModeFlags::None;
	};

	/// @brief Multisample / anti-aliasing state
	struct MultisampleState
	{
		/// @brief Number of samples per pixel (1 = no MSAA)
		uint32_t count = 1;

		/// @brief Bitmask controlling which samples are written
		uint32_t mask = 0xFFFFFFFF;

		/// @brief Enable alpha-to-coverage
		bool alphaToCoverageEnabled = false;
	};

	/// @brief Blend factor
	enum class EBlendFactor : uint32_t
	{
		Zero = 0,
		One,
		Src,
		OneMinusSrc,
		SrcAlpha,
		OneMinusSrcAlpha,
		Dst,
		OneMinusDst,
		DstAlpha,
		OneMinusDstAlpha,
		SrcAlphaSaturated,
		Constant,
		OneMinusConstant,
	};

	/// @brief Blend operation
	enum class EBlendOperation : uint32_t
	{
		Add = 0,
		Subtract,
		ReverseSubtract,
		Min,
		Max,
	};

	/// @brief Color write mask flags
	enum class EColorWriteMask : uint32_t
	{
		None = 0,
		Red = 1 << 0,
		Green = 1 << 1,
		Blue = 1 << 2,
		Alpha = 1 << 3,
		All = Red | Green | Blue | Alpha,
	};

	inline EColorWriteMask operator|(EColorWriteMask a, EColorWriteMask b)
	{
		return static_cast<EColorWriteMask>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EColorWriteMask operator&(EColorWriteMask a, EColorWriteMask b)
	{
		return static_cast<EColorWriteMask>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	/// @brief Describes blend behaviour for a single component (color or alpha)
	struct BlendComponent
	{
		EBlendOperation operation = EBlendOperation::Add;
		EBlendFactor srcFactor = EBlendFactor::One;
		EBlendFactor dstFactor = EBlendFactor::Zero;
	};

	/// @brief Per-target blend state
	struct ColorTargetState
	{
		/// @brief Texture format of this color target
		ETextureFormat format = ETextureFormat::BGRA8_UNORM;

		/// @brief Whether blending is enabled for this target
		bool blendEnabled = false;

		/// @brief Color channel blend settings
		BlendComponent colorBlend;

		/// @brief Alpha channel blend settings
		BlendComponent alphaBlend;

		/// @brief Write mask
		EColorWriteMask writeMask = EColorWriteMask::All;
	};

	/// @brief Comparison function used for depth and stencil tests
	enum class ECompareFunction : uint32_t
	{
		Never = 0,
		Less,
		Equal,
		LessEqual,
		Greater,
		NotEqual,
		GreaterEqual,
		Always,
	};

	/// @brief Stencil operation
	enum class EStencilOperation : uint32_t
	{
		Keep = 0,
		Zero,
		Replace,
		IncrementClamp,
		DecrementClamp,
		Invert,
		IncrementWrap,
		DecrementWrap,
	};

	/// @brief Stencil face state
	struct StencilFaceState
	{
		ECompareFunction compare = ECompareFunction::Always;
		EStencilOperation failOp = EStencilOperation::Keep;
		EStencilOperation depthFailOp = EStencilOperation::Keep;
		EStencilOperation passOp = EStencilOperation::Keep;
	};

	/// @brief Depth/stencil state for the pipeline
	struct DepthStencilState
	{
		/// @brief Whether depth/stencil is enabled at all.
		/// When false, the remaining fields are ignored and no depth/stencil
		/// attachment is expected.
		bool enabled = false;

		/// @brief Depth buffer format
		ETextureFormat format = ETextureFormat::D24_UNORM;

		/// @brief Enable depth testing
		bool depthWriteEnabled = true;

		/// @brief Depth comparison function
		ECompareFunction depthCompare = ECompareFunction::Less;

		/// @brief Stencil front face
		StencilFaceState stencilFront;

		/// @brief Stencil back face
		StencilFaceState stencilBack;

		/// @brief Stencil read mask
		uint32_t stencilReadMask = 0xFF;

		/// @brief Stencil write mask
		uint32_t stencilWriteMask = 0xFF;

		/// @brief Depth bias (constant factor)
		int32_t depthBias = 0;

		/// @brief Depth bias slope factor
		float depthBiasSlopeScale = 0.0f;

		/// @brief Depth bias clamp
		float depthBiasClamp = 0.0f;
	};

	/// @brief Describes a shader stage to attach to a pipeline.
	struct PipelineShaderStage
	{
		/// @brief The compiled shader module
		IShaderModule *module = nullptr;

		/// @brief Entry point name within the module
		/// When empty, the module's own entry point name is used.
		std::string entryPoint;

		/// @brief Returns the effective entry point name.
		[[nodiscard]]
		std::string_view GetEffectiveEntryPoint() const
		{
			if (!entryPoint.empty())
			{
				return entryPoint;
			}
			if (module != nullptr)
			{
				return module->GetEntryPoint();
			}
			return "main";
		}
	};

	/// @brief Maximum number of color targets a pipeline can have.
	static constexpr uint32_t MAX_COLOR_TARGETS = 8;

	/// @brief Maximum number of vertex buffer bindings.
	static constexpr uint32_t MAX_VERTEX_BUFFERS = 16;

	/// @brief Maximum number of bind group layouts per pipeline.
	static constexpr uint32_t MAX_BIND_GROUPS = 4;

	/// @brief Complete descriptor for creating a graphics pipeline.
	///
	/// This encompasses all fixed-function and programmable state required to
	/// create a GPU graphics pipeline state object. Fields have sensible
	/// defaults so that a minimal configuration (vertex + fragment shaders,
	/// one color target format) is sufficient to produce a working pipeline.
	struct GraphicsPipelineDescriptor
	{
		// -- Shader stages --------------------------------------------------

		/// @brief Vertex shader stage (required)
		PipelineShaderStage vertexStage;

		/// @brief Fragment shader stage (required for rasterization pipelines)
		PipelineShaderStage fragmentStage;

		// -- Vertex input ---------------------------------------------------

		/// @brief Vertex buffer layouts describing the vertex input state
		std::vector<VertexBufferLayout> vertexBufferLayouts;

		// -- Primitive state ------------------------------------------------

		PrimitiveState primitive;

		// -- Depth / stencil ------------------------------------------------

		DepthStencilState depthStencil;

		// -- Multisample ----------------------------------------------------

		MultisampleState multisample;

		// -- Color targets (fragment output) ---------------------------------

		/// @brief Color targets that the fragment shader writes to.
		/// At least one is required for a rasterization pipeline.
		std::vector<ColorTargetState> colorTargets;

		// -- Bind group layouts ---------------------------------------------

		/// @brief Bind group layouts that this pipeline uses.
		/// Index in the array corresponds to the set/group number (0-based).
		/// Null entries are allowed and indicate unused groups.
		std::array<IBindGroupLayout *, MAX_BIND_GROUPS> bindGroupLayouts = {};

		/// @brief Number of bind group layouts actually used (starting from
		/// index 0). Entries beyond this count are ignored even if non-null.
		uint32_t bindGroupLayoutCount = 0;

		// -- Debug ----------------------------------------------------------

		/// @brief Optional debug name for the pipeline
		std::string debugName;
	};

	/// @brief Complete descriptor for creating a compute pipeline.
	struct ComputePipelineDescriptor
	{
		/// @brief Compute shader stage (required)
		PipelineShaderStage computeStage;

		/// @brief Bind group layouts that this pipeline uses.
		std::array<IBindGroupLayout *, MAX_BIND_GROUPS> bindGroupLayouts = {};

		/// @brief Number of bind group layouts actually used.
		uint32_t bindGroupLayoutCount = 0;

		/// @brief Optional debug name
		std::string debugName;
	};

} // namespace Hush::Graphics
