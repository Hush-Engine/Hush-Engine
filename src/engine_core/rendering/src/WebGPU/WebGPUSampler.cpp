/*! \file WebGPUSampler.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU implementation of ISampler.
*/

#include "WebGPUSampler.hpp"
#include "WebGPUPipeline.hpp"
#include "Profiling.hpp"

namespace Hush::Graphics
{
	WebGPUSampler::WebGPUSampler(wgpu::Device device, const SamplerDescriptor &descriptor)
		: m_descriptor(descriptor)
	{
		ZoneScoped;
		if (device == nullptr)
		{
			return;
		}

		wgpu::SamplerDescriptor samplerDesc{};

		if (descriptor.debugName != nullptr)
		{
			samplerDesc.label = wgpu::StringView(descriptor.debugName);
		}

		samplerDesc.magFilter = ConvertFilterMode(descriptor.magFilter);
		samplerDesc.minFilter = ConvertFilterMode(descriptor.minFilter);
		samplerDesc.mipmapFilter = ConvertMipmapFilterMode(descriptor.mipmapFilter);

		samplerDesc.addressModeU = ConvertAddressMode(descriptor.addressModeU);
		samplerDesc.addressModeV = ConvertAddressMode(descriptor.addressModeV);
		samplerDesc.addressModeW = ConvertAddressMode(descriptor.addressModeW);

		samplerDesc.lodMinClamp = descriptor.lodMinClamp;
		samplerDesc.lodMaxClamp = descriptor.lodMaxClamp;

		samplerDesc.compare = ConvertCompareFunction(descriptor.compare);

		samplerDesc.maxAnisotropy = descriptor.maxAnisotropy;

		m_sampler = device.createSampler(samplerDesc);
	}

	WebGPUSampler::~WebGPUSampler()
	{
		if (m_sampler != nullptr)
		{
			m_sampler.release();
			m_sampler = nullptr;
		}
	}

	void *WebGPUSampler::GetNativeHandle() const
	{
		// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
		return reinterpret_cast<void *>(static_cast<WGPUSampler>(m_sampler));
	}

} // namespace Hush::Graphics
