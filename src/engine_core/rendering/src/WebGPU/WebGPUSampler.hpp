/*! \file WebGPUSampler.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU implementation of ISampler.
*/
#pragma once

#include "../RHI/ISampler.hpp"
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{
	/// @brief Convert engine filter mode to wgpu::FilterMode.
	inline wgpu::FilterMode ConvertFilterMode(EFilterMode mode)
	{
		switch (mode)
		{
		case EFilterMode::Nearest:
			return wgpu::FilterMode::Nearest;
		case EFilterMode::Linear:
			return wgpu::FilterMode::Linear;
		default:
			return wgpu::FilterMode::Linear;
		}
	}

	/// @brief Convert engine filter mode to wgpu::MipmapFilterMode.
	inline wgpu::MipmapFilterMode ConvertMipmapFilterMode(EFilterMode mode)
	{
		switch (mode)
		{
		case EFilterMode::Nearest:
			return wgpu::MipmapFilterMode::Nearest;
		case EFilterMode::Linear:
			return wgpu::MipmapFilterMode::Linear;
		default:
			return wgpu::MipmapFilterMode::Linear;
		}
	}

	/// @brief Convert engine address mode to wgpu::AddressMode.
	inline wgpu::AddressMode ConvertAddressMode(EAddressMode mode)
	{
		switch (mode)
		{
		case EAddressMode::Repeat:
			return wgpu::AddressMode::Repeat;
		case EAddressMode::MirrorRepeat:
			return wgpu::AddressMode::MirrorRepeat;
		case EAddressMode::ClampToEdge:
			return wgpu::AddressMode::ClampToEdge;
		default:
			return wgpu::AddressMode::ClampToEdge;
		}
	}

	// NOTE: ConvertCompareFunction is defined in WebGPUPipeline.hpp and handles
	// the full ECompareFunction enum including the Undefined value.  Include
	// WebGPUPipeline.hpp in translation units that need the conversion.

	/// @brief WebGPU implementation of ISampler.
	///
	/// Wraps a wgpu::Sampler that controls how textures are sampled
	/// (filtering, addressing, LOD clamping, comparison).
	class WebGPUSampler : public ISampler
	{
	public:
		/// @brief Construct a sampler from the engine descriptor.
		///
		/// Translates the SamplerDescriptor into a wgpu::SamplerDescriptor
		/// and creates the native wgpu::Sampler.
		///
		/// @param device     The WebGPU device.
		/// @param descriptor The backend-agnostic sampler descriptor.
		WebGPUSampler(wgpu::Device device, const SamplerDescriptor &descriptor);

		~WebGPUSampler() override;

		// Non-copyable, non-movable
		WebGPUSampler(const WebGPUSampler &) = delete;
		WebGPUSampler &operator=(const WebGPUSampler &) = delete;
		WebGPUSampler(WebGPUSampler &&) = delete;
		WebGPUSampler &operator=(WebGPUSampler &&) = delete;

		[[nodiscard]]
		EFilterMode GetMagFilter() const override
		{
			return m_descriptor.magFilter;
		}

		[[nodiscard]]
		EFilterMode GetMinFilter() const override
		{
			return m_descriptor.minFilter;
		}

		[[nodiscard]]
		EFilterMode GetMipmapFilter() const override
		{
			return m_descriptor.mipmapFilter;
		}

		[[nodiscard]]
		EAddressMode GetAddressModeU() const override
		{
			return m_descriptor.addressModeU;
		}

		[[nodiscard]]
		EAddressMode GetAddressModeV() const override
		{
			return m_descriptor.addressModeV;
		}

		[[nodiscard]]
		EAddressMode GetAddressModeW() const override
		{
			return m_descriptor.addressModeW;
		}

		[[nodiscard]]
		ECompareFunction GetCompareFunction() const override
		{
			return m_descriptor.compare;
		}

		[[nodiscard]]
		uint16_t GetMaxAnisotropy() const override
		{
			return m_descriptor.maxAnisotropy;
		}

		[[nodiscard]]
		void *GetNativeHandle() const override;

		/// @brief Get the underlying wgpu::Sampler directly.
		[[nodiscard]]
		wgpu::Sampler GetSampler() const
		{
			return m_sampler;
		}

	private:
		/// @brief The native WebGPU sampler.
		wgpu::Sampler m_sampler = nullptr;

		/// @brief Cached descriptor for accessor queries.
		SamplerDescriptor m_descriptor;
	};

} // namespace Hush::Graphics
