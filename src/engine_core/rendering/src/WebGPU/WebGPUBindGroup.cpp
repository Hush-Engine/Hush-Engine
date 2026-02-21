/*! \file WebGPUBindGroup.cpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief WebGPU implementations of IBindGroupLayout and IBindGroup.
*/

#include "WebGPUBindGroup.hpp"
#include "WebGPUBuffer.hpp"
#include "WebGPUTexture.hpp"
#include <vector>

namespace Hush::Graphics
{
	WebGPUBindGroupLayout::WebGPUBindGroupLayout(wgpu::Device device, const BindGroupLayoutDescriptor &descriptor)
		: m_entryCount(static_cast<uint32_t>(descriptor.entries.size())),
		  m_debugName(descriptor.debugName)
	{
		if (device == nullptr)
		{
			return;
		}

		std::vector<wgpu::BindGroupLayoutEntry> entries;
		entries.resize(descriptor.entries.size());

		for (size_t i = 0; i < descriptor.entries.size(); ++i)
		{
			const auto &srcEntry = descriptor.entries[i];
			auto &dstEntry = entries[i];

			// Initialize to default
			dstEntry = wgpu::BindGroupLayoutEntry{};
			dstEntry.binding = srcEntry.binding;
			dstEntry.visibility = ConvertShaderStageFlags(srcEntry.stageFlags);

			switch (srcEntry.type)
			{
			case EBindingType::UniformBuffer: {
				dstEntry.buffer.type = wgpu::BufferBindingType::Uniform;
				dstEntry.buffer.hasDynamicOffset = static_cast<WGPUBool>(srcEntry.hasDynamicOffset);
				dstEntry.buffer.minBindingSize = srcEntry.minBufferBindingSize;
				break;
			}
			case EBindingType::StorageBuffer: {
				dstEntry.buffer.type = wgpu::BufferBindingType::Storage;
				dstEntry.buffer.hasDynamicOffset = static_cast<WGPUBool>(srcEntry.hasDynamicOffset);
				dstEntry.buffer.minBindingSize = srcEntry.minBufferBindingSize;
				break;
			}
			case EBindingType::ReadOnlyStorageBuffer: {
				dstEntry.buffer.type = wgpu::BufferBindingType::ReadOnlyStorage;
				dstEntry.buffer.hasDynamicOffset = static_cast<WGPUBool>(srcEntry.hasDynamicOffset);
				dstEntry.buffer.minBindingSize = srcEntry.minBufferBindingSize;
				break;
			}
			case EBindingType::SampledTexture: {
				dstEntry.texture.sampleType = ConvertTextureSampleType(srcEntry.textureSampleType);
				dstEntry.texture.viewDimension = ConvertTextureViewDimension(srcEntry.textureViewDimension);
				dstEntry.texture.multisampled = static_cast<WGPUBool>(srcEntry.textureMultisampled);
				break;
			}
			case EBindingType::StorageTexture: {
				dstEntry.storageTexture.access = ConvertStorageTextureAccess(srcEntry.storageTextureAccess);
				dstEntry.storageTexture.format = ConvertTextureFormatForBindGroup(srcEntry.storageTextureFormat);
				dstEntry.storageTexture.viewDimension = ConvertTextureViewDimension(srcEntry.textureViewDimension);
				break;
			}
			case EBindingType::Sampler: {
				dstEntry.sampler.type = ConvertSamplerBindingType(srcEntry.samplerType);
				break;
			}
			case EBindingType::ComparisonSampler: {
				dstEntry.sampler.type = wgpu::SamplerBindingType::Comparison;
				break;
			}
			default:
				break;
			}
		}

		wgpu::BindGroupLayoutDescriptor layoutDesc{};

		if (!m_debugName.empty())
		{
			layoutDesc.label = WGPUStringView{m_debugName.c_str(), m_debugName.size()};
		}

		layoutDesc.entryCount = entries.size();
		layoutDesc.entries = entries.data();

		m_layout = device.createBindGroupLayout(layoutDesc);
	}

	WebGPUBindGroupLayout::~WebGPUBindGroupLayout()
	{
		if (m_layout != nullptr)
		{
			m_layout.release();
			m_layout = nullptr;
		}
	}

	WebGPUBindGroup::WebGPUBindGroup(wgpu::Device device, const BindGroupDescriptor &descriptor)
		: m_layout(descriptor.layout),
		  m_debugName(descriptor.debugName)
	{
		if (device == nullptr || descriptor.layout == nullptr)
		{
			return;
		}

		auto *webgpuLayout = dynamic_cast<WebGPUBindGroupLayout *>(descriptor.layout);
		if (webgpuLayout == nullptr || !webgpuLayout->IsValid())
		{
			return;
		}

		std::vector<wgpu::BindGroupEntry> entries;
		entries.resize(descriptor.entries.size());

		for (size_t i = 0; i < descriptor.entries.size(); ++i)
		{
			const auto &srcEntry = descriptor.entries[i];
			auto &dstEntry = entries[i];

			dstEntry = wgpu::BindGroupEntry{};
			dstEntry.binding = srcEntry.binding;

			// Buffer binding
			if (srcEntry.buffer != nullptr)
			{
				auto *webgpuBuffer = dynamic_cast<WebGPUBuffer *>(srcEntry.buffer);
				if (webgpuBuffer != nullptr)
				{
					dstEntry.buffer = webgpuBuffer->GetBuffer();
					dstEntry.offset = srcEntry.offset;

					// If size is 0 or max, use the whole buffer from offset
					if (srcEntry.size == 0 || srcEntry.size == UINT64_MAX)
					{
						dstEntry.size = webgpuBuffer->GetSize() - srcEntry.offset;
					}
					else
					{
						dstEntry.size = srcEntry.size;
					}
				}
			}

			// Texture binding
			if (srcEntry.texture != nullptr)
			{
				auto *webgpuTexture = dynamic_cast<WebGPUTexture *>(srcEntry.texture);
				if (webgpuTexture != nullptr)
				{
					dstEntry.textureView = webgpuTexture->GetView();
				}
			}

			// Sampler binding (opaque pointer — caller passes a wgpu::Sampler*)
			if (srcEntry.sampler != nullptr)
			{
				auto *samplerPtr = static_cast<wgpu::Sampler *>(srcEntry.sampler);
				dstEntry.sampler = *samplerPtr;
			}
		}

		wgpu::BindGroupDescriptor bindGroupDesc{};

		if (!m_debugName.empty())
		{
			bindGroupDesc.label = WGPUStringView{m_debugName.c_str(), m_debugName.size()};
		}

		bindGroupDesc.layout = webgpuLayout->GetLayout();
		bindGroupDesc.entryCount = entries.size();
		bindGroupDesc.entries = entries.data();

		m_bindGroup = device.createBindGroup(bindGroupDesc);
	}

	WebGPUBindGroup::~WebGPUBindGroup()
	{
		if (m_bindGroup != nullptr)
		{
			m_bindGroup.release();
			m_bindGroup = nullptr;
		}
	}

} // namespace Hush::Graphics
