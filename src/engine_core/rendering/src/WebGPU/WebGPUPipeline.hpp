/*! \file WebGPUPipeline.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU implementations of IGraphicsPipeline and IComputePipeline.
*/
#pragma once

#include "../RHI/IPipeline.hpp"
#include "../RHI/PipelineDescriptor.hpp"
#include "../RHI/IShaderModule.hpp"
#include "../RHI/IBindGroup.hpp"
#include "WebGPUShaderModule.hpp"
#include "WebGPUBindGroup.hpp"
#include <string>
#include <vector>
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{
	/// @brief Convert engine vertex format to wgpu::VertexFormat.
	inline wgpu::VertexFormat ConvertVertexFormat(EVertexFormat format)
	{
		switch (format)
		{
		case EVertexFormat::Float32:
			return wgpu::VertexFormat::Float32;
		case EVertexFormat::Float32x2:
			return wgpu::VertexFormat::Float32x2;
		case EVertexFormat::Float32x3:
			return wgpu::VertexFormat::Float32x3;
		case EVertexFormat::Float32x4:
			return wgpu::VertexFormat::Float32x4;

		case EVertexFormat::Sint32:
			return wgpu::VertexFormat::Sint32;
		case EVertexFormat::Sint32x2:
			return wgpu::VertexFormat::Sint32x2;
		case EVertexFormat::Sint32x3:
			return wgpu::VertexFormat::Sint32x3;
		case EVertexFormat::Sint32x4:
			return wgpu::VertexFormat::Sint32x4;

		case EVertexFormat::Uint32:
			return wgpu::VertexFormat::Uint32;
		case EVertexFormat::Uint32x2:
			return wgpu::VertexFormat::Uint32x2;
		case EVertexFormat::Uint32x3:
			return wgpu::VertexFormat::Uint32x3;
		case EVertexFormat::Uint32x4:
			return wgpu::VertexFormat::Uint32x4;

		case EVertexFormat::Float16x2:
			return wgpu::VertexFormat::Float16x2;
		case EVertexFormat::Float16x4:
			return wgpu::VertexFormat::Float16x4;

		case EVertexFormat::Uint8x2:
			return wgpu::VertexFormat::Uint8x2;
		case EVertexFormat::Uint8x4:
			return wgpu::VertexFormat::Uint8x4;

		case EVertexFormat::Sint8x2:
			return wgpu::VertexFormat::Sint8x2;
		case EVertexFormat::Sint8x4:
			return wgpu::VertexFormat::Sint8x4;

		case EVertexFormat::Unorm8x2:
			return wgpu::VertexFormat::Unorm8x2;
		case EVertexFormat::Unorm8x4:
			return wgpu::VertexFormat::Unorm8x4;

		case EVertexFormat::Snorm8x2:
			return wgpu::VertexFormat::Snorm8x2;
		case EVertexFormat::Snorm8x4:
			return wgpu::VertexFormat::Snorm8x4;

		case EVertexFormat::Uint16x2:
			return wgpu::VertexFormat::Uint16x2;
		case EVertexFormat::Uint16x4:
			return wgpu::VertexFormat::Uint16x4;

		case EVertexFormat::Sint16x2:
			return wgpu::VertexFormat::Sint16x2;
		case EVertexFormat::Sint16x4:
			return wgpu::VertexFormat::Sint16x4;

		case EVertexFormat::Unorm16x2:
			return wgpu::VertexFormat::Unorm16x2;
		case EVertexFormat::Unorm16x4:
			return wgpu::VertexFormat::Unorm16x4;

		case EVertexFormat::Snorm16x2:
			return wgpu::VertexFormat::Snorm16x2;
		case EVertexFormat::Snorm16x4:
			return wgpu::VertexFormat::Snorm16x4;

		default:
			return wgpu::VertexFormat::Float32x4;
		}
	}

	/// @brief Convert engine primitive topology to wgpu::PrimitiveTopology.
	inline wgpu::PrimitiveTopology ConvertPrimitiveTopology(EPrimitiveTopology topology)
	{
		switch (topology)
		{
		case EPrimitiveTopology::PointList:
			return wgpu::PrimitiveTopology::PointList;
		case EPrimitiveTopology::LineList:
			return wgpu::PrimitiveTopology::LineList;
		case EPrimitiveTopology::LineStrip:
			return wgpu::PrimitiveTopology::LineStrip;
		case EPrimitiveTopology::TriangleList:
			return wgpu::PrimitiveTopology::TriangleList;
		case EPrimitiveTopology::TriangleStrip:
			return wgpu::PrimitiveTopology::TriangleStrip;
		default:
			return wgpu::PrimitiveTopology::TriangleList;
		}
	}

	/// @brief Convert engine front face to wgpu::FrontFace.
	inline wgpu::FrontFace ConvertFrontFace(EFrontFace frontFace)
	{
		switch (frontFace)
		{
		case EFrontFace::CounterClockwise:
			return wgpu::FrontFace::CCW;
		case EFrontFace::Clockwise:
			return wgpu::FrontFace::CW;
		default:
			return wgpu::FrontFace::CCW;
		}
	}

	/// @brief Convert engine cull mode to wgpu::CullMode.
	inline wgpu::CullMode ConvertCullMode(ECullModeFlags cullMode)
	{
		switch (cullMode)
		{
		case ECullModeFlags::None:
			return wgpu::CullMode::None;
		case ECullModeFlags::Front:
			return wgpu::CullMode::Front;
		case ECullModeFlags::Back:
			return wgpu::CullMode::Back;
		default:
			return wgpu::CullMode::None;
		}
	}

	/// @brief Convert engine index format to wgpu::IndexFormat.
	inline wgpu::IndexFormat ConvertIndexFormat(EIndexFormat format)
	{
		switch (format)
		{
		case EIndexFormat::Uint16:
			return wgpu::IndexFormat::Uint16;
		case EIndexFormat::Uint32:
			return wgpu::IndexFormat::Uint32;
		case EIndexFormat::Undefined:
		default:
			return wgpu::IndexFormat::Undefined;
		}
	}

	/// @brief Convert engine blend factor to wgpu::BlendFactor.
	inline wgpu::BlendFactor ConvertBlendFactor(EBlendFactor factor)
	{
		switch (factor)
		{
		case EBlendFactor::Zero:
			return wgpu::BlendFactor::Zero;
		case EBlendFactor::One:
			return wgpu::BlendFactor::One;
		case EBlendFactor::Src:
			return wgpu::BlendFactor::Src;
		case EBlendFactor::OneMinusSrc:
			return wgpu::BlendFactor::OneMinusSrc;
		case EBlendFactor::SrcAlpha:
			return wgpu::BlendFactor::SrcAlpha;
		case EBlendFactor::OneMinusSrcAlpha:
			return wgpu::BlendFactor::OneMinusSrcAlpha;
		case EBlendFactor::Dst:
			return wgpu::BlendFactor::Dst;
		case EBlendFactor::OneMinusDst:
			return wgpu::BlendFactor::OneMinusDst;
		case EBlendFactor::DstAlpha:
			return wgpu::BlendFactor::DstAlpha;
		case EBlendFactor::OneMinusDstAlpha:
			return wgpu::BlendFactor::OneMinusDstAlpha;
		case EBlendFactor::SrcAlphaSaturated:
			return wgpu::BlendFactor::SrcAlphaSaturated;
		case EBlendFactor::Constant:
			return wgpu::BlendFactor::Constant;
		case EBlendFactor::OneMinusConstant:
			return wgpu::BlendFactor::OneMinusConstant;
		default:
			return wgpu::BlendFactor::One;
		}
	}

	/// @brief Convert engine blend operation to wgpu::BlendOperation.
	inline wgpu::BlendOperation ConvertBlendOperation(EBlendOperation op)
	{
		switch (op)
		{
		case EBlendOperation::Add:
			return wgpu::BlendOperation::Add;
		case EBlendOperation::Subtract:
			return wgpu::BlendOperation::Subtract;
		case EBlendOperation::ReverseSubtract:
			return wgpu::BlendOperation::ReverseSubtract;
		case EBlendOperation::Min:
			return wgpu::BlendOperation::Min;
		case EBlendOperation::Max:
			return wgpu::BlendOperation::Max;
		default:
			return wgpu::BlendOperation::Add;
		}
	}

	/// @brief Convert engine compare function to wgpu::CompareFunction.
	inline wgpu::CompareFunction ConvertCompareFunction(ECompareFunction func)
	{
		switch (func)
		{
		case ECompareFunction::Undefined:
			return wgpu::CompareFunction::Undefined;
		case ECompareFunction::Never:
			return wgpu::CompareFunction::Never;
		case ECompareFunction::Less:
			return wgpu::CompareFunction::Less;
		case ECompareFunction::Equal:
			return wgpu::CompareFunction::Equal;
		case ECompareFunction::LessEqual:
			return wgpu::CompareFunction::LessEqual;
		case ECompareFunction::Greater:
			return wgpu::CompareFunction::Greater;
		case ECompareFunction::NotEqual:
			return wgpu::CompareFunction::NotEqual;
		case ECompareFunction::GreaterEqual:
			return wgpu::CompareFunction::GreaterEqual;
		case ECompareFunction::Always:
			return wgpu::CompareFunction::Always;
		default:
			return wgpu::CompareFunction::Always;
		}
	}

	/// @brief Convert engine stencil operation to wgpu::StencilOperation.
	inline wgpu::StencilOperation ConvertStencilOperation(EStencilOperation op)
	{
		switch (op)
		{
		case EStencilOperation::Keep:
			return wgpu::StencilOperation::Keep;
		case EStencilOperation::Zero:
			return wgpu::StencilOperation::Zero;
		case EStencilOperation::Replace:
			return wgpu::StencilOperation::Replace;
		case EStencilOperation::IncrementClamp:
			return wgpu::StencilOperation::IncrementClamp;
		case EStencilOperation::DecrementClamp:
			return wgpu::StencilOperation::DecrementClamp;
		case EStencilOperation::Invert:
			return wgpu::StencilOperation::Invert;
		case EStencilOperation::IncrementWrap:
			return wgpu::StencilOperation::IncrementWrap;
		case EStencilOperation::DecrementWrap:
			return wgpu::StencilOperation::DecrementWrap;
		default:
			return wgpu::StencilOperation::Keep;
		}
	}

	/// @brief Convert engine texture format to wgpu::TextureFormat.
	///
	/// This is a pipeline-local conversion helper. The WebGPUGraphicsDevice also
	/// has a ConvertTextureFormat but it is private; pipelines need their own
	/// accessible copy to build color target and depth/stencil state descriptors.
	inline wgpu::TextureFormat ConvertTextureFormatForPipeline(ETextureFormat format)
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
			return wgpu::TextureFormat::BGRA8Unorm;
		}
	}

	/// @brief Convert engine color write mask to wgpu::ColorWriteMask.
	inline wgpu::ColorWriteMask ConvertColorWriteMask(EColorWriteMask mask)
	{
		WGPUColorWriteMask result = WGPUColorWriteMask_None;
		if ((static_cast<uint32_t>(mask) & static_cast<uint32_t>(EColorWriteMask::Red)) != 0)
		{
			result |= WGPUColorWriteMask_Red;
		}
		if ((static_cast<uint32_t>(mask) & static_cast<uint32_t>(EColorWriteMask::Green)) != 0)
		{
			result |= WGPUColorWriteMask_Green;
		}
		if ((static_cast<uint32_t>(mask) & static_cast<uint32_t>(EColorWriteMask::Blue)) != 0)
		{
			result |= WGPUColorWriteMask_Blue;
		}
		if ((static_cast<uint32_t>(mask) & static_cast<uint32_t>(EColorWriteMask::Alpha)) != 0)
		{
			result |= WGPUColorWriteMask_Alpha;
		}
		return static_cast<wgpu::ColorWriteMask>(result);
	}

	/// @brief WebGPU implementation of IGraphicsPipeline.
	///
	/// Wraps a wgpu::RenderPipeline created from the engine's
	/// GraphicsPipelineDescriptor. All fixed-function and programmable state is
	/// baked into the native pipeline object at creation time.
	class WebGPUGraphicsPipeline final : public IGraphicsPipeline
	{
	public:
		/// @brief Construct a graphics pipeline from the engine descriptor.
		///
		/// Translates the descriptor into WebGPU-native structures and calls
		/// wgpu::Device::createRenderPipeline.
		///
		/// @param device    The WebGPU device.
		/// @param descriptor The backend-agnostic graphics pipeline descriptor.
		WebGPUGraphicsPipeline(wgpu::Device device, const GraphicsPipelineDescriptor &descriptor)
			: m_debugName(descriptor.debugName),
			  m_colorTargetCount(static_cast<uint32_t>(descriptor.colorTargets.size())),
			  m_hasDepthStencil(descriptor.depthStencil.enabled),
			  m_vertexBufferSlotCount(static_cast<uint32_t>(descriptor.vertexBufferLayouts.size()))
		{
			CreatePipeline(device, descriptor);
		}

		~WebGPUGraphicsPipeline() override
		{
			if (m_pipeline != nullptr)
			{
				m_pipeline.release();
				m_pipeline = nullptr;
			}
		}

		// Non-copyable, non-movable
		WebGPUGraphicsPipeline(const WebGPUGraphicsPipeline &) = delete;
		WebGPUGraphicsPipeline &operator=(const WebGPUGraphicsPipeline &) = delete;
		WebGPUGraphicsPipeline(WebGPUGraphicsPipeline &&) = delete;
		WebGPUGraphicsPipeline &operator=(WebGPUGraphicsPipeline &&) = delete;

		[[nodiscard]]
		bool IsValid() const override
		{
			return m_pipeline != nullptr;
		}

		[[nodiscard]]
		std::string_view GetDebugName() const override
		{
			return m_debugName;
		}

		[[nodiscard]]
		void *GetNativeHandle() const override
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
			return static_cast<void *>(&const_cast<WebGPUGraphicsPipeline *>(this)->m_pipeline);
		}

		[[nodiscard]]
		uint32_t GetColorTargetCount() const override
		{
			return m_colorTargetCount;
		}

		[[nodiscard]]
		bool HasDepthStencil() const override
		{
			return m_hasDepthStencil;
		}

		[[nodiscard]]
		uint32_t GetVertexBufferSlotCount() const override
		{
			return m_vertexBufferSlotCount;
		}

		/// @brief Get the underlying wgpu::RenderPipeline directly.
		[[nodiscard]]
		wgpu::RenderPipeline GetPipeline() const
		{
			return m_pipeline;
		}

	private:
		/// @brief Build and create the wgpu::RenderPipeline from the descriptor.
		void CreatePipeline(wgpu::Device device, const GraphicsPipelineDescriptor &descriptor)
		{
			if (device == nullptr)
			{
				return;
			}

			// ----------------------------------------------------------
			// Vertex state
			// ----------------------------------------------------------

			// We need to keep the attribute arrays alive until createRenderPipeline
			// returns, so we store them in vectors of vectors.
			std::vector<std::vector<wgpu::VertexAttribute>> allAttributes;
			allAttributes.resize(descriptor.vertexBufferLayouts.size());

			std::vector<wgpu::VertexBufferLayout> vertexBuffers;
			vertexBuffers.resize(descriptor.vertexBufferLayouts.size());

			for (size_t vbIdx = 0; vbIdx < descriptor.vertexBufferLayouts.size(); ++vbIdx)
			{
				const auto &srcLayout = descriptor.vertexBufferLayouts[vbIdx];
				auto &dstAttrs = allAttributes[vbIdx];
				dstAttrs.resize(srcLayout.attributes.size());

				for (size_t aIdx = 0; aIdx < srcLayout.attributes.size(); ++aIdx)
				{
					dstAttrs[aIdx].shaderLocation = srcLayout.attributes[aIdx].shaderLocation;
					dstAttrs[aIdx].offset = static_cast<uint64_t>(srcLayout.attributes[aIdx].offset);
					dstAttrs[aIdx].format = ConvertVertexFormat(srcLayout.attributes[aIdx].format);
				}

				vertexBuffers[vbIdx].arrayStride = static_cast<uint64_t>(srcLayout.stride);
				vertexBuffers[vbIdx].stepMode = (srcLayout.stepMode == EVertexStepMode::Instance)
													? wgpu::VertexStepMode::Instance
													: wgpu::VertexStepMode::Vertex;
				vertexBuffers[vbIdx].attributeCount = dstAttrs.size();
				vertexBuffers[vbIdx].attributes = dstAttrs.data();
			}

			// Vertex shader module
			auto *vsModule = dynamic_cast<WebGPUShaderModule *>(descriptor.vertexStage.module);
			if (vsModule == nullptr || !vsModule->IsValid())
			{
				return;
			}

			wgpu::VertexState vertexState{};
			vertexState.module = vsModule->GetModule();
			auto vsEntry = descriptor.vertexStage.GetEffectiveEntryPoint();
			vertexState.entryPoint = WGPUStringView{vsEntry.data(), vsEntry.size()};
			vertexState.bufferCount = vertexBuffers.size();
			vertexState.buffers = vertexBuffers.data();

			// ----------------------------------------------------------
			// Primitive state
			// ----------------------------------------------------------

			wgpu::PrimitiveState primitiveState{};
			primitiveState.topology = ConvertPrimitiveTopology(descriptor.primitive.topology);
			primitiveState.frontFace = ConvertFrontFace(descriptor.primitive.frontFace);
			primitiveState.cullMode = ConvertCullMode(descriptor.primitive.cullMode);

			if (descriptor.primitive.topology == EPrimitiveTopology::TriangleStrip ||
				descriptor.primitive.topology == EPrimitiveTopology::LineStrip)
			{
				primitiveState.stripIndexFormat = ConvertIndexFormat(descriptor.primitive.stripIndexFormat);
			}

			// ----------------------------------------------------------
			// Multisample state
			// ----------------------------------------------------------

			wgpu::MultisampleState multisampleState{};
			multisampleState.count = descriptor.multisample.count;
			multisampleState.mask = descriptor.multisample.mask;
			multisampleState.alphaToCoverageEnabled =
				static_cast<WGPUBool>(descriptor.multisample.alphaToCoverageEnabled);

			// ----------------------------------------------------------
			// Fragment state + color targets
			// ----------------------------------------------------------

			std::vector<wgpu::ColorTargetState> colorTargets;
			std::vector<wgpu::BlendState> blendStates; // keep alive
			colorTargets.resize(descriptor.colorTargets.size());
			blendStates.resize(descriptor.colorTargets.size());

			for (size_t i = 0; i < descriptor.colorTargets.size(); ++i)
			{
				const auto &srcTarget = descriptor.colorTargets[i];
				colorTargets[i].format = ConvertTextureFormatForPipeline(srcTarget.format);
				colorTargets[i].writeMask = ConvertColorWriteMask(srcTarget.writeMask);

				if (srcTarget.blendEnabled)
				{
					blendStates[i].color.operation = ConvertBlendOperation(srcTarget.colorBlend.operation);
					blendStates[i].color.srcFactor = ConvertBlendFactor(srcTarget.colorBlend.srcFactor);
					blendStates[i].color.dstFactor = ConvertBlendFactor(srcTarget.colorBlend.dstFactor);

					blendStates[i].alpha.operation = ConvertBlendOperation(srcTarget.alphaBlend.operation);
					blendStates[i].alpha.srcFactor = ConvertBlendFactor(srcTarget.alphaBlend.srcFactor);
					blendStates[i].alpha.dstFactor = ConvertBlendFactor(srcTarget.alphaBlend.dstFactor);

					colorTargets[i].blend = &blendStates[i];
				}
				else
				{
					colorTargets[i].blend = nullptr;
				}
			}

			// Fragment shader module
			auto *fsModule = dynamic_cast<WebGPUShaderModule *>(descriptor.fragmentStage.module);

			wgpu::FragmentState fragmentState{};
			bool hasFragment = (fsModule != nullptr && fsModule->IsValid());

			if (hasFragment)
			{
				fragmentState.module = fsModule->GetModule();
				auto fsEntry = descriptor.fragmentStage.GetEffectiveEntryPoint();
				fragmentState.entryPoint = WGPUStringView{fsEntry.data(), fsEntry.size()};
				fragmentState.targetCount = colorTargets.size();
				fragmentState.targets = colorTargets.data();
			}

			// ----------------------------------------------------------
			// Depth/stencil state
			// ----------------------------------------------------------

			wgpu::DepthStencilState depthStencilState{};

			if (descriptor.depthStencil.enabled)
			{
				depthStencilState.format = ConvertTextureFormatForPipeline(descriptor.depthStencil.format);
				depthStencilState.depthWriteEnabled = descriptor.depthStencil.depthWriteEnabled
														  ? wgpu::OptionalBool(WGPUOptionalBool_True)
														  : wgpu::OptionalBool(WGPUOptionalBool_False);
				depthStencilState.depthCompare = ConvertCompareFunction(descriptor.depthStencil.depthCompare);

				depthStencilState.stencilFront.compare =
					ConvertCompareFunction(descriptor.depthStencil.stencilFront.compare);
				depthStencilState.stencilFront.failOp =
					ConvertStencilOperation(descriptor.depthStencil.stencilFront.failOp);
				depthStencilState.stencilFront.depthFailOp =
					ConvertStencilOperation(descriptor.depthStencil.stencilFront.depthFailOp);
				depthStencilState.stencilFront.passOp =
					ConvertStencilOperation(descriptor.depthStencil.stencilFront.passOp);

				depthStencilState.stencilBack.compare =
					ConvertCompareFunction(descriptor.depthStencil.stencilBack.compare);
				depthStencilState.stencilBack.failOp =
					ConvertStencilOperation(descriptor.depthStencil.stencilBack.failOp);
				depthStencilState.stencilBack.depthFailOp =
					ConvertStencilOperation(descriptor.depthStencil.stencilBack.depthFailOp);
				depthStencilState.stencilBack.passOp =
					ConvertStencilOperation(descriptor.depthStencil.stencilBack.passOp);

				depthStencilState.stencilReadMask = descriptor.depthStencil.stencilReadMask;
				depthStencilState.stencilWriteMask = descriptor.depthStencil.stencilWriteMask;
				depthStencilState.depthBias = descriptor.depthStencil.depthBias;
				depthStencilState.depthBiasSlopeScale = descriptor.depthStencil.depthBiasSlopeScale;
				depthStencilState.depthBiasClamp = descriptor.depthStencil.depthBiasClamp;
			}

			// ----------------------------------------------------------
			// Pipeline layout (from bind group layouts)
			// ----------------------------------------------------------

			wgpu::PipelineLayout pipelineLayout = nullptr;

			if (descriptor.bindGroupLayoutCount > 0)
			{
				std::vector<wgpu::BindGroupLayout> bgLayouts;
				bgLayouts.reserve(descriptor.bindGroupLayoutCount);

				for (uint32_t i = 0; i < descriptor.bindGroupLayoutCount; ++i)
				{
					if (descriptor.bindGroupLayouts[i] != nullptr)
					{
						auto *webgpuLayout = dynamic_cast<WebGPUBindGroupLayout *>(descriptor.bindGroupLayouts[i]);
						if (webgpuLayout != nullptr)
						{
							bgLayouts.push_back(webgpuLayout->GetLayout());
						}
					}
				}

				if (!bgLayouts.empty())
				{
					std::string layoutLabel;
					wgpu::PipelineLayoutDescriptor layoutDesc{};
					layoutDesc.bindGroupLayoutCount = bgLayouts.size();
					layoutDesc.bindGroupLayouts = reinterpret_cast<const WGPUBindGroupLayout *>(bgLayouts.data());

					if (!m_debugName.empty())
					{
						layoutLabel = m_debugName + "_layout";
						layoutDesc.label = WGPUStringView{layoutLabel.c_str(), layoutLabel.size()};
					}

					pipelineLayout = device.createPipelineLayout(layoutDesc);
				}
			}

			// ----------------------------------------------------------
			// Assemble the render pipeline descriptor
			// ----------------------------------------------------------

			wgpu::RenderPipelineDescriptor pipelineDesc{};

			if (!m_debugName.empty())
			{
				pipelineDesc.label = WGPUStringView{m_debugName.c_str(), m_debugName.size()};
			}

			pipelineDesc.vertex = vertexState;
			pipelineDesc.primitive = primitiveState;
			pipelineDesc.multisample = multisampleState;

			if (hasFragment)
			{
				pipelineDesc.fragment = &fragmentState;
			}

			if (descriptor.depthStencil.enabled)
			{
				pipelineDesc.depthStencil = &depthStencilState;
			}

			if (pipelineLayout != nullptr)
			{
				pipelineDesc.layout = pipelineLayout;
			}

			// ----------------------------------------------------------
			// Create the pipeline
			// ----------------------------------------------------------

			m_pipeline = device.createRenderPipeline(pipelineDesc);

			// Release the temporary pipeline layout (the pipeline holds its own ref)
			if (pipelineLayout != nullptr)
			{
				pipelineLayout.release();
			}
		}

		/// @brief The compiled WebGPU render pipeline.
		wgpu::RenderPipeline m_pipeline = nullptr;

		/// @brief Debug name for identification.
		std::string m_debugName;

		/// @brief Number of color targets this pipeline was created with.
		uint32_t m_colorTargetCount = 0;

		/// @brief Whether depth/stencil state was enabled.
		bool m_hasDepthStencil = false;

		/// @brief Number of vertex buffer slots expected.
		uint32_t m_vertexBufferSlotCount = 0;
	};

	/// @brief WebGPU implementation of IComputePipeline.
	///
	/// Wraps a wgpu::ComputePipeline created from the engine's
	/// ComputePipelineDescriptor.
	class WebGPUComputePipeline final : public IComputePipeline
	{
	public:
		/// @brief Construct a compute pipeline from the engine descriptor.
		///
		/// @param device     The WebGPU device.
		/// @param descriptor The backend-agnostic compute pipeline descriptor.
		WebGPUComputePipeline(wgpu::Device device, const ComputePipelineDescriptor &descriptor)
			: m_debugName(descriptor.debugName)
		{
			CreatePipeline(device, descriptor);
		}

		~WebGPUComputePipeline() override
		{
			if (m_pipeline != nullptr)
			{
				m_pipeline.release();
				m_pipeline = nullptr;
			}
		}

		// Non-copyable, non-movable
		WebGPUComputePipeline(const WebGPUComputePipeline &) = delete;
		WebGPUComputePipeline &operator=(const WebGPUComputePipeline &) = delete;
		WebGPUComputePipeline(WebGPUComputePipeline &&) = delete;
		WebGPUComputePipeline &operator=(WebGPUComputePipeline &&) = delete;

		[[nodiscard]]
		bool IsValid() const override
		{
			return m_pipeline != nullptr;
		}

		[[nodiscard]]
		std::string_view GetDebugName() const override
		{
			return m_debugName;
		}

		[[nodiscard]]
		void *GetNativeHandle() const override
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
			return static_cast<void *>(&const_cast<WebGPUComputePipeline *>(this)->m_pipeline);
		}

		/// @brief Get the underlying wgpu::ComputePipeline directly.
		[[nodiscard]]
		wgpu::ComputePipeline GetPipeline() const
		{
			return m_pipeline;
		}

	private:
		/// @brief Build and create the wgpu::ComputePipeline from the descriptor.
		void CreatePipeline(wgpu::Device device, const ComputePipelineDescriptor &descriptor)
		{
			if (device == nullptr)
			{
				return;
			}

			auto *csModule = dynamic_cast<WebGPUShaderModule *>(descriptor.computeStage.module);
			if (csModule == nullptr || !csModule->IsValid())
			{
				return;
			}

			// ----------------------------------------------------------
			// Pipeline layout (from bind group layouts)
			// ----------------------------------------------------------

			wgpu::PipelineLayout pipelineLayout = nullptr;

			if (descriptor.bindGroupLayoutCount > 0)
			{
				std::vector<wgpu::BindGroupLayout> bgLayouts;
				bgLayouts.reserve(descriptor.bindGroupLayoutCount);

				for (uint32_t i = 0; i < descriptor.bindGroupLayoutCount; ++i)
				{
					if (descriptor.bindGroupLayouts[i] != nullptr)
					{
						auto *webgpuLayout = dynamic_cast<WebGPUBindGroupLayout *>(descriptor.bindGroupLayouts[i]);
						if (webgpuLayout != nullptr)
						{
							bgLayouts.push_back(webgpuLayout->GetLayout());
						}
					}
				}

				if (!bgLayouts.empty())
				{
					std::string layoutLabel;
					wgpu::PipelineLayoutDescriptor layoutDesc{};
					layoutDesc.bindGroupLayoutCount = bgLayouts.size();
					layoutDesc.bindGroupLayouts = reinterpret_cast<const WGPUBindGroupLayout *>(bgLayouts.data());

					if (!m_debugName.empty())
					{
						layoutLabel = m_debugName + "_layout";
						layoutDesc.label = WGPUStringView{layoutLabel.c_str(), layoutLabel.size()};
					}

					pipelineLayout = device.createPipelineLayout(layoutDesc);
				}
			}

			// ----------------------------------------------------------
			// Compute pipeline descriptor
			// ----------------------------------------------------------

			wgpu::ComputePipelineDescriptor pipelineDesc{};

			if (!m_debugName.empty())
			{
				pipelineDesc.label = WGPUStringView{m_debugName.c_str(), m_debugName.size()};
			}

			pipelineDesc.compute.module = csModule->GetModule();
			auto csEntry = descriptor.computeStage.GetEffectiveEntryPoint();
			pipelineDesc.compute.entryPoint = WGPUStringView{csEntry.data(), csEntry.size()};

			if (pipelineLayout != nullptr)
			{
				pipelineDesc.layout = pipelineLayout;
			}

			// ----------------------------------------------------------
			// Create the pipeline
			// ----------------------------------------------------------

			m_pipeline = device.createComputePipeline(pipelineDesc);

			// Release the temporary pipeline layout
			if (pipelineLayout != nullptr)
			{
				pipelineLayout.release();
			}
		}

		/// @brief The compiled WebGPU compute pipeline.
		wgpu::ComputePipeline m_pipeline = nullptr;

		/// @brief Debug name for identification.
		std::string m_debugName;
	};

} // namespace Hush::Graphics
