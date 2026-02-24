/*! \file WebGPUTexture.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU texture implementation
*/
#include "WebGPUTexture.hpp"

Hush::Graphics::WebGPUTexture::WebGPUTexture(wgpu::Texture texture, wgpu::TextureView view,
											 const TextureDescriptor &desc)
	: m_texture(texture),
	  m_view(view),
	  m_descriptor(desc)
{
}

Hush::Graphics::WebGPUTexture::WebGPUTexture(WebGPUTexture &&rhs) noexcept
	: m_texture(std::move(rhs.m_texture)),
	  m_view(std::move(rhs.m_view)),
	  m_descriptor(rhs.m_descriptor)
{
	rhs.m_texture = nullptr;
	rhs.m_view = nullptr;
	rhs.m_descriptor = {};
}

Hush::Graphics::WebGPUTexture &Hush::Graphics::WebGPUTexture::operator=(WebGPUTexture &&rhs) noexcept
{
	if (this != &rhs)
	{
		m_texture = std::move(rhs.m_texture);
		m_view = std::move(rhs.m_view);
		m_descriptor = rhs.m_descriptor;
		rhs.m_texture = nullptr;
		rhs.m_view = nullptr;
		rhs.m_descriptor = {};
	}
	return *this;
}

Hush::Graphics::WebGPUTexture::~WebGPUTexture()
{
	m_view = nullptr;

	if (m_texture != nullptr && !m_descriptor.ownedByExternalSource)
	{
		m_texture.destroy();
	}
}

void *Hush::Graphics::WebGPUTexture::GetNativeHandle() const
{
	return static_cast<void *>(static_cast<WGPUTexture>(m_texture));
}
