/*! \file WebGPUBuffer.cpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU buffer implementation
*/
#include "WebGPUBuffer.hpp"
#include "Logger.hpp"

namespace Hush::Graphics
{

WebGPUBuffer::WebGPUBuffer(wgpu::Buffer buffer, const BufferDescriptor& desc)
	: m_buffer(buffer)
	, m_descriptor(desc)
{
}

WebGPUBuffer::~WebGPUBuffer()
{
	if (m_mappedData != nullptr)
	{
		Unmap();
	}
	m_buffer.destroy();
}

void* WebGPUBuffer::Map()
{
	if (m_mappedData != nullptr)
	{
		return m_mappedData;
	}

	// Note: WebGPU mapping is async, simplified here for basic usage
	// In production, you'd use mapAsync with callbacks
	LogWarn("WebGPU buffer mapping not fully implemented");
	return nullptr;
}

void WebGPUBuffer::Unmap()
{
	if (m_mappedData == nullptr)
	{
		return;
	}

	m_buffer.unmap();
	m_mappedData = nullptr;
}

void* WebGPUBuffer::GetNativeHandle() const
{
	return static_cast<void*>(static_cast<WGPUBuffer>(m_buffer));
}

} // namespace Hush::Graphics
