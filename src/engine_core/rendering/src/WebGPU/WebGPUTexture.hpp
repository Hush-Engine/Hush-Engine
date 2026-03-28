/*! \file WebGPUTexture.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU texture implementation
*/
#pragma once

#include "../RHI/IGraphicsTexture.hpp"
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{
	/// @brief WebGPU texture implementation
	class WebGPUTexture : public IGraphicsTexture
	{
		friend class WebGPUGraphicsDevice;

		WebGPUTexture() = default;

	public:
		WebGPUTexture(wgpu::Texture texture, wgpu::TextureView view, const TextureDescriptor &desc);
		~WebGPUTexture() override;

		WebGPUTexture(const WebGPUTexture &) = delete;
		WebGPUTexture(WebGPUTexture &&rhs) noexcept;
		WebGPUTexture &operator=(const WebGPUTexture &) = delete;
		WebGPUTexture &operator=(WebGPUTexture &&) noexcept;

		[[nodiscard]]
		uint32_t GetWidth() const override
		{
			return m_descriptor.width;
		}

		[[nodiscard]]
		uint32_t GetHeight() const override
		{
			return m_descriptor.height;
		}

		[[nodiscard]]
		uint32_t GetDepth() const override
		{
			return m_descriptor.depth;
		}

		[[nodiscard]]
		ETextureFormat GetFormat() const override
		{
			return m_descriptor.format;
		}

		[[nodiscard]]
		uint32_t GetMipLevels() const override
		{
			return m_descriptor.mipLevels;
		}

		[[nodiscard]]
		void *GetNativeHandle() const override;

		[[nodiscard]]
		void *GetNativeView() const override;

		[[nodiscard]]
		wgpu::Texture GetTexture() const
		{
			return m_texture;
		}
		[[nodiscard]]
		wgpu::TextureView GetView() const
		{
			return m_view;
		}

	private:
		wgpu::Texture m_texture;
		wgpu::TextureView m_view;
		TextureDescriptor m_descriptor;
	};

} // namespace Hush::Graphics
