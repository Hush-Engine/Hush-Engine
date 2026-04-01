/*! \file Material3D.cpp
	\date 2026-02-20
	\brief Material3D implementation — pipeline/bind-group creation, uniform
		   property management, and command-list binding.

	Material3D does NOT compile shaders.  It receives pre-compiled
	IShaderModule pointers and a ShaderCompilationResult (for reflection)
	through the Material3DDescriptor.
*/

#include "Material3D.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include "RHI/ShaderCompiler.hpp"
#include "Logger.hpp"

#include <algorithm>
#include <cstring>

namespace Hush::Graphics
{
	// =====================================================================
	// Initialisation
	// =====================================================================

	std::optional<Material3D::EError> Material3D::Init(IGraphicsDevice *device, const Material3DDescriptor &descriptor)
	{
		if (device == nullptr)
		{
			LogFormat(ELogLevel::Error, "[Material3D] Init called with null device.");
			return EError::NullDevice;
		}

		if (descriptor.vertexShader == nullptr || descriptor.fragmentShader == nullptr)
		{
			LogFormat(ELogLevel::Error, "[Material3D] Init called with null shader module(s).");
			return EError::NullShaderModule;
		}

		if (!descriptor.vertexShader->IsValid() || !descriptor.fragmentShader->IsValid())
		{
			LogFormat(ELogLevel::Error, "[Material3D] Init called with invalid shader module(s).");
			return EError::InvalidShaderModule;
		}

		m_name = descriptor.debugName;

		std::vector<BindGroupLayoutDescriptor> layoutDescs;

		if (descriptor.compilationResult != nullptr)
		{
			layoutDescs = descriptor.compilationResult->BuildBindGroupLayoutDescriptors();
			BuildPropertyMapFromReflection(*descriptor.compilationResult);
		}

		EError bufferAndBindGroupError = CreateUniformBufferAndBindGroup(device, layoutDescs);
		if (bufferAndBindGroupError != EError::None)
		{
			return bufferAndBindGroupError;
		}

		EError pipelineError = CreatePipeline(device, descriptor);
		if (pipelineError != EError::None)
		{
			return pipelineError;
		}

		m_internalMaterial.pipeline = m_pipeline.Get();
		m_internalMaterial.bindGroup = m_bindGroup.Get();
		m_internalMaterial.passType = m_materialPass;

		m_initialized = true;

		LogFormat(ELogLevel::Info,
				  "[Material3D] '%s': initialised successfully (uniform buffer: %llu bytes, "
				  "properties: %zu).",
				  m_name.c_str(), static_cast<unsigned long long>(m_uniformStagingBuffer.size()), m_propertyMap.size());

		return std::nullopt;
	}

	bool Material3D::IsInitialized() const noexcept
	{
		return m_initialized;
	}

	Material3D::EError Material3D::CreateUniformBufferAndBindGroup(IGraphicsDevice *device,
																   std::vector<BindGroupLayoutDescriptor> &layoutDescs)
	{
		// Compute the total uniform buffer size from the property map.
		uint64_t uniformBufferSize = 0;
		for (const auto &[propName, propInfo] : m_propertyMap)
		{
			const uint64_t end = static_cast<uint64_t>(propInfo.offset) + propInfo.size;
			uniformBufferSize = std::max(uniformBufferSize, end);
		}

		// Also honour the reflected buffer size from bind group layout entries.
		if (!layoutDescs.empty())
		{
			for (auto &entry : layoutDescs[0].entries)
			{
				if (entry.type == EBindingType::UniformBuffer)
				{
					uniformBufferSize = std::max(uniformBufferSize, entry.minBufferBindingSize);
					if (entry.minBufferBindingSize == 0 && uniformBufferSize > 0)
					{
						entry.minBufferBindingSize = uniformBufferSize;
					}
				}
			}
		}

		// Fallback: if reflection produced nothing, create a minimal layout
		// with a single uniform buffer entry.
		if (layoutDescs.empty())
		{
			BindGroupLayoutDescriptor manualLayout{};
			manualLayout.debugName = m_name + "_BindGroupLayout";
			if (uniformBufferSize > 0)
			{
				BindGroupLayoutEntry uniformEntry{};
				uniformEntry.binding = 0;
				uniformEntry.type = EBindingType::UniformBuffer;
				uniformEntry.stageFlags = EShaderStageFlags::Vertex | EShaderStageFlags::Fragment;
				uniformEntry.minBufferBindingSize = uniformBufferSize;
				manualLayout.entries.push_back(uniformEntry);
			}
			layoutDescs.push_back(std::move(manualLayout));
		}

		layoutDescs[0].debugName = m_name + "_BindGroupLayout0";

		m_bindGroupLayout.CreateResource(layoutDescs[0], device);
		if (!m_bindGroupLayout.IsValid())
		{
			LogFormat(ELogLevel::Error, "[Material3D] '%s': failed to create bind group layout.", m_name.c_str());
			return EError::BindGroupLayoutCreationFailed;
		}

		// Ensure at least 16 bytes so WebGPU does not reject a zero-size buffer.
		uniformBufferSize = std::max(uniformBufferSize, static_cast<uint64_t>(16));

		// Round up to 16 bytes for GPU alignment.
		uniformBufferSize = (uniformBufferSize + 15u) & ~static_cast<uint64_t>(15u);

		// Persist the debug name so the pointer stays valid during CreateResource.
		std::string bufferDebugName = m_name + "_UniformBuffer";

		BufferDescriptor bufferDesc{};
		bufferDesc.size = uniformBufferSize;
		bufferDesc.usage = EBufferUsage::Uniform;
		bufferDesc.memoryAccess = EMemoryAccess::CPUWrite;
		bufferDesc.debugName = bufferDebugName.c_str();

		m_uniformBuffer.CreateResource(bufferDesc, device);
		if (!m_uniformBuffer.IsValid())
		{
			LogFormat(ELogLevel::Error, "[Material3D] '%s': failed to create uniform buffer.", m_name.c_str());
			return EError::UniformBufferCreationFailed;
		}

		// Initialise the CPU staging buffer with zeros and upload.
		m_uniformStagingBuffer.resize(static_cast<size_t>(uniformBufferSize), 0);
		device->WriteBuffer(m_uniformBuffer.Get(), 0, m_uniformStagingBuffer.data(), uniformBufferSize);

		// Create the bind group.
		BindGroupDescriptor bgDesc{};
		bgDesc.layout = m_bindGroupLayout.Get();
		bgDesc.debugName = m_name + "_BindGroup";

		BindGroupEntry bufEntry{};
		bufEntry.binding = 0;
		bufEntry.buffer = m_uniformBuffer.Get();
		bufEntry.offset = 0;
		bufEntry.size = uniformBufferSize;
		bgDesc.entries.push_back(bufEntry);

		m_bindGroup.CreateResource(bgDesc, device);
		if (!m_bindGroup.IsValid())
		{
			LogFormat(ELogLevel::Error, "[Material3D] '%s': failed to create bind group.", m_name.c_str());
			return EError::BindGroupCreationFailed;
		}

		return EError::None;
	}

	Material3D::EError Material3D::CreatePipeline(IGraphicsDevice *device, const Material3DDescriptor &descriptor)
	{
		GraphicsPipelineDescriptor pipelineDesc{};
		pipelineDesc.debugName = m_name + "_Pipeline";

		// Shader stages — non-owning pointers from the descriptor.
		pipelineDesc.vertexStage.module = descriptor.vertexShader;
		pipelineDesc.vertexStage.entryPoint = descriptor.vertexEntry;

		pipelineDesc.fragmentStage.module = descriptor.fragmentShader;
		pipelineDesc.fragmentStage.entryPoint = descriptor.fragmentEntry;

		// Vertex input from reflection (when available).
		if (descriptor.compilationResult != nullptr && !descriptor.compilationResult->vertexInputs.empty())
		{
			VertexBufferLayout vbl{};
			uint32_t currentOffset = 0;
			for (const auto &input : descriptor.compilationResult->vertexInputs)
			{
				VertexAttribute attr{};
				attr.shaderLocation = input.location;
				attr.offset = currentOffset;
				attr.format = input.format;
				currentOffset += GetVertexFormatSize(input.format);
				vbl.attributes.push_back(attr);
			}
			vbl.stride = currentOffset;
			vbl.stepMode = EVertexStepMode::Vertex;
			pipelineDesc.vertexBufferLayouts.push_back(std::move(vbl));
		}

		// Primitive / rasterisation state
		pipelineDesc.primitive.topology = EPrimitiveTopology::TriangleList;
		pipelineDesc.primitive.frontFace = EFrontFace::CounterClockwise;
		pipelineDesc.primitive.cullMode = TranslateCullMode(m_cullMode);

		// Color target
		pipelineDesc.colorTargets.push_back(
			BuildColorTarget(descriptor.colorTargetFormat, m_alphaBlendMode, descriptor.blendEnabled));

		// Depth/stencil
		pipelineDesc.depthStencil = descriptor.depthStencil;

		// Bind group layouts
		pipelineDesc.bindGroupLayouts[0] = m_bindGroupLayout.Get();
		pipelineDesc.bindGroupLayoutCount = 1;

		m_pipeline.CreateResource(pipelineDesc, device);
		if (!m_pipeline.IsValid())
		{
			LogFormat(ELogLevel::Error, "[Material3D] '%s': failed to create graphics pipeline.", m_name.c_str());
			return EError::PipelineCreationFailed;
		}

		return EError::None;
	}

	void Material3D::FlushProperties(IGraphicsDevice *device)
	{
		if (!m_propertiesDirty || !m_initialized || device == nullptr)
		{
			return;
		}

		device->WriteBuffer(m_uniformBuffer.Get(), 0, m_uniformStagingBuffer.data(),
							static_cast<uint64_t>(m_uniformStagingBuffer.size()));

		m_propertiesDirty = false;
	}

	// =====================================================================
	// Rendering
	// =====================================================================

	void Material3D::Bind(IGraphicsCommandList *cmdList, uint32_t bindGroupIndex) const
	{
		if (cmdList == nullptr || !m_initialized)
		{
			return;
		}

		if (m_pipeline.IsValid())
		{
			cmdList->BindPipeline(m_pipeline.Get());
		}

		if (m_bindGroup.IsValid())
		{
			cmdList->SetBindGroup(bindGroupIndex, m_bindGroup.Get());
		}
	}

	// =====================================================================
	// Material options
	// =====================================================================

	EAlphaBlendMode Material3D::GetAlphaBlendMode() const noexcept
	{
		return m_alphaBlendMode;
	}

	void Material3D::SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept
	{
		m_alphaBlendMode = blendMode;
	}

	ECullMode Material3D::GetCullMode() const noexcept
	{
		return m_cullMode;
	}

	void Material3D::SetCullMode(ECullMode cullMode) noexcept
	{
		m_cullMode = cullMode;
	}

	EMaterialPass Material3D::GetMaterialPass() const noexcept
	{
		return m_materialPass;
	}

	void Material3D::SetMaterialPass(EMaterialPass pass) noexcept
	{
		m_materialPass = pass;
		m_internalMaterial.passType = pass;
	}

	// =====================================================================
	// Internal material instance
	// =====================================================================

	GraphicsApiMaterialInstance *Material3D::GetInternalMaterial() noexcept
	{
		return m_initialized ? &m_internalMaterial : nullptr;
	}

	const GraphicsApiMaterialInstance *Material3D::GetInternalMaterial() const noexcept
	{
		return m_initialized ? &m_internalMaterial : nullptr;
	}

	// =====================================================================
	// Name
	// =====================================================================

	void Material3D::SetName(std::string_view name)
	{
		m_name = name;
	}

	const std::string &Material3D::GetName() const noexcept
	{
		return m_name;
	}

	// =====================================================================
	// Resource accessors
	// =====================================================================

	IGraphicsPipeline *Material3D::GetPipeline() const noexcept
	{
		return m_pipeline.IsValid() ? m_pipeline.Get() : nullptr;
	}

	IBindGroup *Material3D::GetBindGroup() const noexcept
	{
		return m_bindGroup.IsValid() ? m_bindGroup.Get() : nullptr;
	}

	IBindGroupLayout *Material3D::GetBindGroupLayout() const noexcept
	{
		return m_bindGroupLayout.IsValid() ? m_bindGroupLayout.Get() : nullptr;
	}

	IGraphicsBuffer *Material3D::GetUniformBuffer() const noexcept
	{
		return m_uniformBuffer.IsValid() ? m_uniformBuffer.Get() : nullptr;
	}

	const std::unordered_map<std::string, MaterialPropertyInfo> &Material3D::GetPropertyMap() const noexcept
	{
		return m_propertyMap;
	}

	uint64_t Material3D::GetUniformBufferSize() const noexcept
	{
		return static_cast<uint64_t>(m_uniformStagingBuffer.size());
	}

	// =====================================================================
	// Private helpers
	// =====================================================================

	void Material3D::BuildPropertyMapFromReflection(const ShaderCompilationResult &result)
	{
		m_propertyMap.clear();

		// The Slang-based ShaderCompiler populates `result.bindings` with a flat
		// list of ReflectedBinding entries.  Uniform buffer members are reported
		// with their offset (via `binding` index) and size (via `bufferSize`).
		//
		// For a ConstantBuffer<Uniforms> with members { float4x4 mvp; float4 tint; float time; }
		// the compiler may emit one top-level binding of type UniformBuffer whose
		// `bufferSize` is the total size.  In that case the individual member
		// offsets are not directly available from the coarse reflection data,
		// so we store the whole buffer as a single property at offset 0.
		//
		// If the compiler emits per-member bindings (some Slang configurations
		// do), we store each member individually.

		uint32_t runningOffset = 0;

		for (const auto &b : result.bindings)
		{
			if (b.type != EBindingType::UniformBuffer)
			{
				continue;
			}

			MaterialPropertyInfo info{};
			info.offset = runningOffset;
			info.size = b.bufferSize > 0 ? static_cast<uint32_t>(b.bufferSize) : 0;

			if (!b.name.empty())
			{
				m_propertyMap[b.name] = info;
			}

			if (info.size > 0)
			{
				runningOffset += info.size;
			}
		}
	}

	ColorTargetState Material3D::BuildColorTarget(ETextureFormat format, EAlphaBlendMode blendMode, bool blendEnabled)
	{
		ColorTargetState target{};
		target.format = format;
		target.writeMask = EColorWriteMask::All;
		target.blendEnabled = blendEnabled;

		if (!blendEnabled || blendMode == EAlphaBlendMode::None)
		{
			target.blendEnabled = false;
			return target;
		}

		// Color blend: always use standard alpha blending operation (Add).
		target.colorBlend.operation = EBlendOperation::Add;
		target.alphaBlend.operation = EBlendOperation::Add;

		switch (blendMode)
		{
		case EAlphaBlendMode::SrcAlpha:
			target.colorBlend.srcFactor = EBlendFactor::SrcAlpha;
			target.colorBlend.dstFactor = EBlendFactor::OneMinusSrcAlpha;
			target.alphaBlend.srcFactor = EBlendFactor::One;
			target.alphaBlend.dstFactor = EBlendFactor::OneMinusSrcAlpha;
			break;

		case EAlphaBlendMode::OneMinusSrcAlpha:
			target.colorBlend.srcFactor = EBlendFactor::OneMinusSrcAlpha;
			target.colorBlend.dstFactor = EBlendFactor::SrcAlpha;
			target.alphaBlend.srcFactor = EBlendFactor::OneMinusSrcAlpha;
			target.alphaBlend.dstFactor = EBlendFactor::One;
			break;

		case EAlphaBlendMode::OneMinusDestAlpha:
			target.colorBlend.srcFactor = EBlendFactor::OneMinusDstAlpha;
			target.colorBlend.dstFactor = EBlendFactor::DstAlpha;
			target.alphaBlend.srcFactor = EBlendFactor::OneMinusDstAlpha;
			target.alphaBlend.dstFactor = EBlendFactor::One;
			break;

		case EAlphaBlendMode::DestAlpha:
			target.colorBlend.srcFactor = EBlendFactor::DstAlpha;
			target.colorBlend.dstFactor = EBlendFactor::OneMinusDstAlpha;
			target.alphaBlend.srcFactor = EBlendFactor::DstAlpha;
			target.alphaBlend.dstFactor = EBlendFactor::One;
			break;

		case EAlphaBlendMode::ConstAlpha:
			target.colorBlend.srcFactor = EBlendFactor::Constant;
			target.colorBlend.dstFactor = EBlendFactor::OneMinusConstant;
			target.alphaBlend.srcFactor = EBlendFactor::Constant;
			target.alphaBlend.dstFactor = EBlendFactor::OneMinusConstant;
			break;

		case EAlphaBlendMode::None:
		default:
			target.blendEnabled = false;
			break;
		}

		return target;
	}

	ECullModeFlags Material3D::TranslateCullMode(ECullMode mode) noexcept
	{
		switch (mode)
		{
		case ECullMode::Front:
			return ECullModeFlags::Front;
		case ECullMode::Back:
			return ECullModeFlags::Back;
		default:
			return ECullModeFlags::None;
		}
	}

} // namespace Hush::Graphics
