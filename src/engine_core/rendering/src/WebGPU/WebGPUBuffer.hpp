/*! \file WebGPUBuffer.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU buffer implementation
*/
#pragma once

#include "../RHI/IGraphicsBuffer.hpp"
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{
	/// @brief WebGPU buffer implementation
	class WebGPUBuffer : public IGraphicsBuffer
	{
	public:
		WebGPUBuffer(wgpu::Buffer buffer, wgpu::Instance instance, const BufferDescriptor &desc);
		~WebGPUBuffer() override;

		WebGPUBuffer(const WebGPUBuffer &) = delete;
		WebGPUBuffer(WebGPUBuffer &&) = delete;
		WebGPUBuffer &operator=(const WebGPUBuffer &) = delete;
		WebGPUBuffer &operator=(WebGPUBuffer &&) = delete;

		[[nodiscard]]
		uint64_t GetSize() const override
		{
			return m_descriptor.size;
		}
		[[nodiscard]]
		EBufferUsage GetUsage() const override
		{
			return m_descriptor.usage;
		}
		void *Map(Graphics::IGraphicsDevice *device) override;
		void Unmap() override;
		[[nodiscard]]
		void *GetNativeHandle() const override;

		[[nodiscard]]
		wgpu::Buffer GetBuffer() const
		{
			return m_buffer;
		}

	private:
		wgpu::Buffer m_buffer;
		wgpu::Instance m_instance;
		BufferDescriptor m_descriptor;
		void *m_mappedData = nullptr;
	};

} // namespace Hush::Graphics
