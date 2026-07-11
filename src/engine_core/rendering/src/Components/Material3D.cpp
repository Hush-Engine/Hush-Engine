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
#include <string>

namespace Hush::Graphics
{
	// =====================================================================
	// Initialisation
	// =====================================================================

	Material3D::EError Material3D::Init(IGraphicsDevice *device, const Material3DDescriptor &descriptor)
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

			// Determine which set the material's uniform properties belong to.
			if (!m_propertyMap.empty())
			{
				m_materialBindGroupSet = m_propertyMap.begin()->second.bindingSet;
			}
		}

		EError layoutErr = CreateAllBindGroupLayouts(device, layoutDescs);
		if (layoutErr != EError::None)
		{
			return layoutErr;
		}

		EError bufErr = CreateUniformBufferAndBindGroup(device);
		if (bufErr != EError::None)
		{
			return bufErr;
		}

		EError pipelineErr = CreatePipeline(device, descriptor);
		if (pipelineErr != EError::None)
		{
			return pipelineErr;
		}

		m_internalMaterial.pipeline = m_pipeline.Get();
		m_internalMaterial.bindGroup = m_bindGroup.Get();
		m_internalMaterial.passType = m_materialPass;

		m_initialized = true;

		LogFormat(ELogLevel::Info,
				  "[Material3D] '%s': initialised successfully (uniform buffer: %llu bytes, "
				  "properties: %zu, sets: %zu).",
				  m_name.c_str(), static_cast<unsigned long long>(m_uniformStagingBuffer.size()), m_propertyMap.size(),
				  m_bindGroupLayouts.size());

		return EError::None;
	}

	bool Material3D::IsInitialized() const noexcept
	{
		return m_initialized;
	}

	Material3D::EError Material3D::SetPropertyRaw(std::string_view name, const std::span<const std::byte> &value)
	{
		auto it = m_propertyMap.find(std::string(name));
		if (it == m_propertyMap.end())
		{
			return EError::PropertyNotFound;
		}

		const MaterialPropertyInfo &info = it->second;
		const size_t writeSize = value.size_bytes() < info.size ? value.size_bytes() : info.size;

		if (info.offset + writeSize > m_uniformStagingBuffer.size())
		{
			return EError::PropertyNotFound;
		}

		std::memcpy(m_uniformStagingBuffer.data() + info.offset, value.data(), writeSize);
		m_propertiesDirty = true;
		return EError::None;
	}
	Material3D::EError Material3D::CreateAllBindGroupLayouts(IGraphicsDevice *device,
															 const std::vector<BindGroupLayoutDescriptor> &layoutDescs)
	{
		const size_t setCount = layoutDescs.size();

		// Fallback: no reflection data — create a single minimal layout.
		if (setCount == 0)
		{
			m_bindGroupLayouts.resize(1);

			BindGroupLayoutDescriptor manualLayout{};
			manualLayout.debugName = m_name + "_BindGroupLayout";

			BindGroupLayoutEntry uniformEntry{};
			uniformEntry.binding = 0;
			uniformEntry.type = EBindingType::UniformBuffer;
			uniformEntry.stageFlags = EShaderStageFlags::Vertex | EShaderStageFlags::Fragment;
			manualLayout.entries.push_back(uniformEntry);

			m_bindGroupLayouts[0].CreateResource(manualLayout, device);
			if (!m_bindGroupLayouts[0].IsValid())
			{
				LogFormat(ELogLevel::Error, "[Material3D] '%s': failed to create fallback bind group layout.",
						  m_name.c_str());
				return EError::BindGroupLayoutCreationFailed;
			}

			m_materialBindGroupSet = 0;
			return EError::None;
		}

		m_bindGroupLayouts.resize(setCount);

		for (size_t i = 0; i < setCount; ++i)
		{
			// Non-material set: use the full reflected layout as-is.
			if (i != static_cast<size_t>(m_materialBindGroupSet))
			{
				m_bindGroupLayouts[i].CreateResource(layoutDescs[i], device);
				continue;
			}

			// Material set: filter to only entries Material3D actually provides.
			BindGroupLayoutDescriptor filtered{};
			filtered.debugName = m_name + "_MaterialBindGroupLayout";

			for (const auto &entry : layoutDescs[i].entries)
			{
				if (entry.type != EBindingType::UniformBuffer)
				{
					continue;
				}
				filtered.entries.push_back(entry);
			}

			m_bindGroupLayouts[i].CreateResource(filtered, device);
		}

		// Validate all layouts.
		for (size_t i = 0; i < setCount; ++i)
		{
			if (!m_bindGroupLayouts[i].IsValid())
			{
				LogFormat(ELogLevel::Error, "[Material3D] '%s': failed to create bind group layout for set %zu.",
						  m_name.c_str(), i);
				return EError::BindGroupLayoutCreationFailed;
			}
		}

		return EError::None;
	}

	Material3D::EError Material3D::CreateUniformBufferAndBindGroup(IGraphicsDevice *device)
	{
		// Compute the total uniform buffer size from the property map.
		uint64_t uniformBufferSize = 0;
		for (const auto &[propName, propInfo] : m_propertyMap)
		{
			const uint64_t end = static_cast<uint64_t>(propInfo.offset) + propInfo.size;
			uniformBufferSize = std::max(uniformBufferSize, end);
		}

		// Honour the staging buffer size already computed by BuildPropertyMapFromReflection.
		uniformBufferSize = std::max(uniformBufferSize, static_cast<uint64_t>(m_uniformStagingBuffer.size()));

		// Ensure at least 16 bytes so WebGPU does not reject a zero-size buffer.
		uniformBufferSize = std::max(uniformBufferSize, static_cast<uint64_t>(16));

		// Round up to 16 bytes for GPU alignment.
		uniformBufferSize = (uniformBufferSize + 15u) & ~static_cast<uint64_t>(15u);

		std::string bufferDebugName = m_name + "_UniformBuffer";

		BufferDescriptor bufferDesc{};
		bufferDesc.size = uniformBufferSize;
		bufferDesc.usage = EBufferUsage::Uniform;
		bufferDesc.memoryAccess = EMemoryAccess::CPUNone;
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

		// Determine the binding index from the first uniform property.
		uint32_t uniformBinding = 0;
		if (!m_propertyMap.empty())
		{
			uniformBinding = m_propertyMap.begin()->second.binding;
		}

		const size_t matSet = static_cast<size_t>(m_materialBindGroupSet);

		// Create the bind group using the material set's filtered layout.
		BindGroupDescriptor bgDesc{};
		bgDesc.layout = m_bindGroupLayouts[matSet].Get();
		bgDesc.debugName = m_name + "_BindGroup";

		BindGroupEntry bufEntry{};
		bufEntry.binding = uniformBinding;
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

	Material3D::EError Material3D::CreatePipeline(IGraphicsDevice *device, const Material3DDescriptor &descriptor)
	{
		GraphicsPipelineDescriptor pipelineDesc{};
		pipelineDesc.debugName = m_name + "_Pipeline";

		pipelineDesc.vertexStage.module = descriptor.vertexShader;
		pipelineDesc.vertexStage.entryPoint = descriptor.vertexEntry;

		pipelineDesc.fragmentStage.module = descriptor.fragmentShader;
		pipelineDesc.fragmentStage.entryPoint = descriptor.fragmentEntry;

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

		pipelineDesc.primitive.topology = EPrimitiveTopology::TriangleList;
		pipelineDesc.primitive.frontFace = EFrontFace::CounterClockwise;
		pipelineDesc.primitive.cullMode = TranslateCullMode(m_cullMode);

		pipelineDesc.colorTargets.push_back(
			BuildColorTarget(descriptor.colorTargetFormat, m_alphaBlendMode, descriptor.blendEnabled));

		pipelineDesc.depthStencil = descriptor.depthStencil;

		const uint32_t layoutCount = std::min(static_cast<uint32_t>(m_bindGroupLayouts.size()), MAX_BIND_GROUPS);
		for (uint32_t i = 0; i < layoutCount; ++i)
		{
			pipelineDesc.bindGroupLayouts[i] = m_bindGroupLayouts[i].Get();
		}
		pipelineDesc.bindGroupLayoutCount = layoutCount;

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

	IBindGroupLayout *Material3D::GetBindGroupLayout(uint32_t setIndex) const noexcept
	{
		if (setIndex >= m_bindGroupLayouts.size())
		{
			return nullptr;
		}
		return m_bindGroupLayouts[setIndex].IsValid() ? m_bindGroupLayouts[setIndex].Get() : nullptr;
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

	const std::vector<uint8_t> &Material3D::GetUniformStagingBuffer() const noexcept
	{
		return m_uniformStagingBuffer;
	}

	// =====================================================================
	// Private helpers
	// =====================================================================

	void Material3D::BuildPropertyMapFromReflection(const ShaderCompilationResult &result)
	{
		m_propertyMap.clear();

		uint32_t maxEnd = 0;

		// First pass: process per-member entries (isMember flag set by ShaderCompiler
		// when reflecting ConstantBuffer struct fields).  These carry exact offsets
		// reported by Slang and must NOT contribute to a running-offset accumulator —
		// the top-level parent entry already accounts for the full buffer size.
		for (const auto &b : result.bindings)
		{
			if (b.type != EBindingType::UniformBuffer || !b.isMember)
			{
				continue;
			}

			MaterialPropertyInfo info{};
			info.offset = static_cast<uint32_t>(b.bufferOffset);
			info.size = static_cast<uint32_t>(b.bufferSize);
			info.bindingSet = b.set;
			info.binding = b.binding;

			if (!b.name.empty())
			{
				m_propertyMap[b.name] = info;
			}

			maxEnd = std::max(maxEnd, info.offset + info.size);
		}

		// Second pass: process top-level parent entries (isMember == false).
		// These provide the accurate total size of each ConstantBuffer and also
		// serve as fallback properties when no per-member entries were emitted.
		for (const auto &b : result.bindings)
		{
			if (b.type != EBindingType::UniformBuffer || b.isMember)
			{
				continue;
			}

			maxEnd = std::max(maxEnd, static_cast<uint32_t>(b.bufferSize));

			if (b.name.empty())
			{
				continue;
			}

			// Check whether per-member entries already exist for this logical buffer.
			bool hasMembers = false;
			for (const auto &inner : result.bindings)
			{
				if (inner.isMember && inner.set == b.set && inner.binding == b.binding)
				{
					hasMembers = true;
					break;
				}
			}

			if (hasMembers)
			{
				continue;
			}

			// No per-member entries — store the whole buffer as one property.
			MaterialPropertyInfo fallbackInfo{};
			fallbackInfo.offset = 0;
			fallbackInfo.size = static_cast<uint32_t>(b.bufferSize);
			fallbackInfo.bindingSet = b.set;
			fallbackInfo.binding = b.binding;
			m_propertyMap[b.name] = fallbackInfo;
		}

		// Ensure at least 16 bytes and 16-byte alignment for GPU.
		maxEnd = std::max(maxEnd, 16u);
		maxEnd = (maxEnd + 15u) & ~static_cast<uint32_t>(15u);

		m_uniformStagingBuffer.resize(maxEnd, 0);
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
