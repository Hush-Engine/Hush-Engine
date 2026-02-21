/*! \file WebGPUCommandList.cpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU implementation of command list interfaces
*/
#include "WebGPUCommandList.hpp"
#include "WebGPUBuffer.hpp"
#include "WebGPUTexture.hpp"
#include "WebGPUPipeline.hpp"
#include "WebGPUBindGroup.hpp"
#include "Assertions.hpp"
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{

	static wgpu::TextureFormat GetTextureFormat(IGraphicsTexture *texture)
	{
		auto *webgpuTexture = dynamic_cast<WebGPUTexture *>(texture);
		HUSH_ASSERT(webgpuTexture != nullptr, "Texture must be a WebGPUTexture");

		using Hush::Graphics::ETextureUsage;

		// Map ETextureFormat to wgpu::TextureFormat
		switch (webgpuTexture->GetFormat())
		{
		case ETextureFormat::R8_UNORM:
			return wgpu::TextureFormat::R8Unorm;
		case ETextureFormat::R8_SNORM:
			return wgpu::TextureFormat::R8Snorm;
		case ETextureFormat::R16_FLOAT:
			return wgpu::TextureFormat::R16Float;
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
		case ETextureFormat::RGBA32_FLOAT:
			return wgpu::TextureFormat::RGBA32Float;
		case ETextureFormat::D32_FLOAT:
			return wgpu::TextureFormat::Depth32Float;
		case ETextureFormat::D24_UNORM_S8_UINT:
			return wgpu::TextureFormat::Depth24PlusStencil8;
		default:
			return wgpu::TextureFormat::Undefined;
		}
	}

	static uint32_t GetTextureBytesPerPixel(wgpu::TextureFormat format)
	{
		switch (format)
		{
		case wgpu::TextureFormat::R8Unorm:
		case wgpu::TextureFormat::R8Snorm:
			return 1;
		case wgpu::TextureFormat::R16Float:
		case wgpu::TextureFormat::RG8Unorm:
		case wgpu::TextureFormat::RG8Snorm:
			return 2;
		case wgpu::TextureFormat::R32Float:
		case wgpu::TextureFormat::RG16Float:
		case wgpu::TextureFormat::RGBA8Unorm:
		case wgpu::TextureFormat::RGBA8UnormSrgb:
		case wgpu::TextureFormat::RGBA8Snorm:
		case wgpu::TextureFormat::BGRA8Unorm:
		case wgpu::TextureFormat::BGRA8UnormSrgb:
		case wgpu::TextureFormat::Depth32Float:
		case wgpu::TextureFormat::Depth24PlusStencil8:
			return 4;
		case wgpu::TextureFormat::RG32Float:
		case wgpu::TextureFormat::RGBA16Float:
			return 8;
		case wgpu::TextureFormat::RGBA32Float:
			return 16;
		default:
			return 4;
		}
	}

	WebGPUCopyCommandList::WebGPUCopyCommandList(wgpu::Device device)
		: m_device(device)
	{
		Reset();
	}

	WebGPUCopyCommandList::~WebGPUCopyCommandList()
	{
		if (m_isRecording && m_encoder != nullptr)
		{
			m_encoder.release();
		}
	}

	void WebGPUCopyCommandList::Reset()
	{
		if (m_isRecording && m_encoder != nullptr)
		{
			m_encoder.release();
		}

		m_commandBuffer = nullptr;

		wgpu::CommandEncoderDescriptor encoderDesc{};
		encoderDesc.label = WGPUStringView("Copy Command Encoder");
		m_encoder = m_device.createCommandEncoder(encoderDesc);
		m_isRecording = true;
	}

	void WebGPUCopyCommandList::Close()
	{
		if (!m_isRecording)
		{
			return;
		}

		wgpu::CommandBufferDescriptor cmdBufferDesc{};
		cmdBufferDesc.label = WGPUStringView("Copy Command Buffer");
		m_commandBuffer = m_encoder.finish(cmdBufferDesc);
		m_isRecording = false;
	}

	void *WebGPUCopyCommandList::GetNativeHandle() const
	{
		return static_cast<void *>(static_cast<WGPUCommandBuffer>(m_commandBuffer));
	}

	// WebGPU manages resource transitions internally — all barrier methods are noops.

	void WebGPUCopyCommandList::ResourceBarrier(std::span<const ResourceBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU handles resource state transitions automatically.
	}

	void WebGPUCopyCommandList::UAVBarrier(std::span<const UAVBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU handles UAV synchronization automatically.
	}

	void WebGPUCopyCommandList::BeginSplitBarrier(std::span<const SplitBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU does not support split barriers.
	}

	void WebGPUCopyCommandList::EndSplitBarrier(std::span<const SplitBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU does not support split barriers.
	}

	void WebGPUCopyCommandList::CopyBuffer(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsBuffer *dst,
										   uint64_t dstOffset, uint64_t size)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(src && dst, "Source and destination buffers must be valid");

		auto *srcBuffer = dynamic_cast<WebGPUBuffer *>(src);
		auto *dstBuffer = dynamic_cast<WebGPUBuffer *>(dst);

		m_encoder.copyBufferToBuffer(srcBuffer->GetBuffer(), srcOffset, dstBuffer->GetBuffer(), dstOffset, size);
	}

	void WebGPUCopyCommandList::CopyBufferToTexture(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsTexture *dst,
													uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width,
													uint32_t height, uint32_t depth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(src && dst, "Source buffer and destination texture must be valid");

		auto *srcBuffer = dynamic_cast<WebGPUBuffer *>(src);
		auto *dstTexture = dynamic_cast<WebGPUTexture *>(dst);

		wgpu::TexelCopyBufferInfo source{};
		source.buffer = srcBuffer->GetBuffer();
		source.layout.offset = srcOffset;
		source.layout.bytesPerRow = width * GetTextureBytesPerPixel(GetTextureFormat(dst));
		source.layout.rowsPerImage = height;

		wgpu::TexelCopyTextureInfo destination{};
		destination.texture = dstTexture->GetTexture();
		destination.mipLevel = 0;
		destination.origin = {.x = dstX, .y = dstY, .z = dstZ};
		destination.aspect = wgpu::TextureAspect::All;

		wgpu::Extent3D copySize{width, height, depth};

		m_encoder.copyBufferToTexture(source, destination, copySize);
	}

	void WebGPUCopyCommandList::CopyTextureToBuffer(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
													IGraphicsBuffer *dst, uint64_t dstOffset, uint32_t width,
													uint32_t height, uint32_t depth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(src && dst, "Source texture and destination buffer must be valid");

		auto *srcTexture = dynamic_cast<WebGPUTexture *>(src);
		auto *dstBuffer = dynamic_cast<WebGPUBuffer *>(dst);

		wgpu::TexelCopyTextureInfo source{};
		source.texture = srcTexture->GetTexture();
		source.mipLevel = 0;
		source.origin = {.x = srcX, .y = srcY, .z = srcZ};
		source.aspect = wgpu::TextureAspect::All;

		wgpu::TexelCopyBufferInfo destination{};
		destination.buffer = dstBuffer->GetBuffer();
		destination.layout.offset = dstOffset;
		destination.layout.bytesPerRow = width * GetTextureBytesPerPixel(GetTextureFormat(src));
		destination.layout.rowsPerImage = height;

		wgpu::Extent3D copySize{width, height, depth};

		m_encoder.copyTextureToBuffer(source, destination, copySize);
	}

	void WebGPUCopyCommandList::CopyTexture(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
											IGraphicsTexture *dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
											uint32_t width, uint32_t height, uint32_t depth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(src && dst, "Source and destination textures must be valid");

		auto *srcTexture = dynamic_cast<WebGPUTexture *>(src);
		auto *dstTexture = dynamic_cast<WebGPUTexture *>(dst);

		wgpu::TexelCopyTextureInfo source{};
		source.texture = srcTexture->GetTexture();
		source.mipLevel = 0;
		source.origin = {.x = srcX, .y = srcY, .z = srcZ};
		source.aspect = wgpu::TextureAspect::All;

		wgpu::TexelCopyTextureInfo destination{};
		destination.texture = dstTexture->GetTexture();
		destination.mipLevel = 0;
		destination.origin = {.x = dstX, .y = dstY, .z = dstZ};
		destination.aspect = wgpu::TextureAspect::All;

		wgpu::Extent3D copySize{width, height, depth};

		m_encoder.copyTextureToTexture(source, destination, copySize);
	}

	WebGPUComputeCommandList::WebGPUComputeCommandList(wgpu::Device device)
		: m_device(device)
	{
		Reset();
	}

	WebGPUComputeCommandList::~WebGPUComputeCommandList()
	{
		if (m_inComputePass && m_computePass != nullptr)
		{
			m_computePass.end();
		}
		if (m_isRecording && m_encoder != nullptr)
		{
			m_encoder.release();
		}
	}

	void WebGPUComputeCommandList::Reset()
	{
		if (m_inComputePass && m_computePass != nullptr)
		{
			m_computePass.end();
			m_inComputePass = false;
		}

		if (m_isRecording && m_encoder != nullptr)
		{
			m_encoder.release();
		}

		m_commandBuffer = nullptr;

		wgpu::CommandEncoderDescriptor encoderDesc{};
		encoderDesc.label = WGPUStringView("Compute Command Encoder");
		m_encoder = m_device.createCommandEncoder(encoderDesc);
		m_isRecording = true;
	}

	void WebGPUComputeCommandList::Close()
	{
		if (!m_isRecording)
		{
			return;
		}

		if (m_inComputePass && m_computePass != nullptr)
		{
			m_computePass.end();
			m_inComputePass = false;
		}

		wgpu::CommandBufferDescriptor cmdBufferDesc{};
		cmdBufferDesc.label = WGPUStringView("Compute Command Buffer");
		m_commandBuffer = m_encoder.finish(cmdBufferDesc);
		m_isRecording = false;
	}

	void *WebGPUComputeCommandList::GetNativeHandle() const
	{
		return static_cast<void *>(static_cast<WGPUCommandBuffer>(m_commandBuffer));
	}

	// WebGPU manages resource transitions internally — all barrier methods are noops.

	void WebGPUComputeCommandList::ResourceBarrier(std::span<const ResourceBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU handles resource state transitions automatically.
	}

	void WebGPUComputeCommandList::UAVBarrier(std::span<const UAVBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU handles UAV synchronization automatically.
	}

	void WebGPUComputeCommandList::BeginSplitBarrier(std::span<const SplitBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU does not support split barriers.
	}

	void WebGPUComputeCommandList::EndSplitBarrier(std::span<const SplitBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU does not support split barriers.
	}

	void WebGPUComputeCommandList::CopyBuffer(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsBuffer *dst,
											  uint64_t dstOffset, uint64_t size)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inComputePass, "Cannot copy during compute pass");
		HUSH_ASSERT(src && dst, "Source and destination buffers must be valid");

		auto *srcBuffer = dynamic_cast<WebGPUBuffer *>(src);
		auto *dstBuffer = dynamic_cast<WebGPUBuffer *>(dst);

		m_encoder.copyBufferToBuffer(srcBuffer->GetBuffer(), srcOffset, dstBuffer->GetBuffer(), dstOffset, size);
	}

	void WebGPUComputeCommandList::CopyBufferToTexture(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsTexture *dst,
													   uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width,
													   uint32_t height, uint32_t depth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inComputePass, "Cannot copy during compute pass");
		HUSH_ASSERT(src && dst, "Source buffer and destination texture must be valid");

		auto *srcBuffer = dynamic_cast<WebGPUBuffer *>(src);
		auto *dstTexture = dynamic_cast<WebGPUTexture *>(dst);

		wgpu::TexelCopyBufferInfo source{};
		source.buffer = srcBuffer->GetBuffer();
		source.layout.offset = srcOffset;
		source.layout.bytesPerRow = width * GetTextureBytesPerPixel(GetTextureFormat(dst));
		source.layout.rowsPerImage = height;

		wgpu::TexelCopyTextureInfo destination{};
		destination.texture = dstTexture->GetTexture();
		destination.mipLevel = 0;
		destination.origin = {.x = dstX, .y = dstY, .z = dstZ};
		destination.aspect = wgpu::TextureAspect::All;

		wgpu::Extent3D copySize{width, height, depth};

		m_encoder.copyBufferToTexture(source, destination, copySize);
	}

	void WebGPUComputeCommandList::CopyTextureToBuffer(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY,
													   uint32_t srcZ, IGraphicsBuffer *dst, uint64_t dstOffset,
													   uint32_t width, uint32_t height, uint32_t depth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inComputePass, "Cannot copy during compute pass");
		HUSH_ASSERT(src && dst, "Source texture and destination buffer must be valid");

		auto *srcTexture = dynamic_cast<WebGPUTexture *>(src);
		auto *dstBuffer = dynamic_cast<WebGPUBuffer *>(dst);

		wgpu::TexelCopyTextureInfo source{};
		source.texture = srcTexture->GetTexture();
		source.mipLevel = 0;
		source.origin = {.x = srcX, .y = srcY, .z = srcZ};
		source.aspect = wgpu::TextureAspect::All;

		wgpu::TexelCopyBufferInfo destination{};
		destination.buffer = dstBuffer->GetBuffer();
		destination.layout.offset = dstOffset;
		destination.layout.bytesPerRow = width * GetTextureBytesPerPixel(GetTextureFormat(src));
		destination.layout.rowsPerImage = height;

		wgpu::Extent3D copySize{width, height, depth};

		m_encoder.copyTextureToBuffer(source, destination, copySize);
	}

	void WebGPUComputeCommandList::CopyTexture(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
											   IGraphicsTexture *dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
											   uint32_t width, uint32_t height, uint32_t depth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inComputePass, "Cannot copy during compute pass");
		HUSH_ASSERT(src && dst, "Source and destination textures must be valid");

		auto *srcTexture = dynamic_cast<WebGPUTexture *>(src);
		auto *dstTexture = dynamic_cast<WebGPUTexture *>(dst);

		wgpu::TexelCopyTextureInfo source{};
		source.texture = srcTexture->GetTexture();
		source.mipLevel = 0;
		source.origin = {.x = srcX, .y = srcY, .z = srcZ};
		source.aspect = wgpu::TextureAspect::All;

		wgpu::TexelCopyTextureInfo destination{};
		destination.texture = dstTexture->GetTexture();
		destination.mipLevel = 0;
		destination.origin = {.x = dstX, .y = dstY, .z = dstZ};
		destination.aspect = wgpu::TextureAspect::All;

		wgpu::Extent3D copySize{width, height, depth};

		m_encoder.copyTextureToTexture(source, destination, copySize);
	}

	void WebGPUComputeCommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");

		// Begin compute pass if not already in one
		if (!m_inComputePass)
		{
			wgpu::ComputePassDescriptor computePassDesc{};
			computePassDesc.label = WGPUStringView("Compute Pass");
			m_computePass = m_encoder.beginComputePass(computePassDesc);
			m_inComputePass = true;
		}

		m_computePass.dispatchWorkgroups(groupCountX, groupCountY, groupCountZ);
	}

	void WebGPUComputeCommandList::DispatchIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(indirectArgsBuffer, "Indirect args buffer must be valid");

		auto *buffer = dynamic_cast<WebGPUBuffer *>(indirectArgsBuffer);

		// Begin compute pass if not already in one
		if (!m_inComputePass)
		{
			wgpu::ComputePassDescriptor computePassDesc{};
			computePassDesc.label = WGPUStringView("Compute Pass", WGPU_STRLEN);
			m_computePass = m_encoder.beginComputePass(computePassDesc);
			m_inComputePass = true;
		}

		m_computePass.dispatchWorkgroupsIndirect(buffer->GetBuffer(), offset);
	}

	void WebGPUComputeCommandList::BindComputePipeline(IComputePipeline *pipeline)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(pipeline, "Pipeline must be valid");

		// Begin compute pass if not already in one
		if (!m_inComputePass)
		{
			wgpu::ComputePassDescriptor computePassDesc{};
			computePassDesc.label = WGPUStringView("Compute Pass");
			m_computePass = m_encoder.beginComputePass(computePassDesc);
			m_inComputePass = true;
		}

		auto *webgpuPipeline = dynamic_cast<WebGPUComputePipeline *>(pipeline);
		HUSH_ASSERT(webgpuPipeline, "Pipeline must be a WebGPU compute pipeline");
		m_computePass.setPipeline(webgpuPipeline->GetPipeline());
	}

	void WebGPUComputeCommandList::SetComputeBindGroup(uint32_t groupIndex, IBindGroup *bindGroup,
													   std::span<const uint32_t> dynamicOffsets)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inComputePass, "Must be in a compute pass to set bind group");
		HUSH_ASSERT(bindGroup, "Bind group must be valid");

		auto *webgpuBindGroup = dynamic_cast<WebGPUBindGroup *>(bindGroup);
		HUSH_ASSERT(webgpuBindGroup, "Bind group must be a WebGPU bind group");
		m_computePass.setBindGroup(groupIndex, webgpuBindGroup->GetBindGroup(), dynamicOffsets.size(),
								   dynamicOffsets.data());
	}

	WebGPUGraphicsCommandList::WebGPUGraphicsCommandList(wgpu::Device device)
		: m_device(device)
	{
		Reset();
	}

	WebGPUGraphicsCommandList::~WebGPUGraphicsCommandList()
	{
		if (m_inRenderPass && m_renderPass != nullptr)
		{
			m_renderPass.end();
		}
		if (m_inComputePass && m_computePass != nullptr)
		{
			m_computePass.end();
		}
		if (m_isRecording && m_encoder != nullptr)
		{
			m_encoder.release();
		}
	}

	void WebGPUGraphicsCommandList::Reset()
	{
		if (m_inRenderPass && m_renderPass != nullptr)
		{
			m_renderPass.end();
			m_inRenderPass = false;
		}

		if (m_inComputePass && m_computePass != nullptr)
		{
			m_computePass.end();
			m_inComputePass = false;
		}

		if (m_isRecording && m_encoder != nullptr)
		{
			m_encoder.release();
		}

		m_commandBuffer = nullptr;

		wgpu::CommandEncoderDescriptor encoderDesc{};
		encoderDesc.label = WGPUStringView("Graphics Command Encoder", WGPU_STRLEN);
		m_encoder = m_device.createCommandEncoder(encoderDesc);
		m_isRecording = true;
	}

	void WebGPUGraphicsCommandList::Close()
	{
		if (!m_isRecording)
		{
			return;
		}

		if (m_inRenderPass && m_renderPass != nullptr)
		{
			m_renderPass.end();
			m_inRenderPass = false;
		}

		if (m_inComputePass && m_computePass != nullptr)
		{
			m_computePass.end();
			m_inComputePass = false;
		}

		wgpu::CommandBufferDescriptor cmdBufferDesc{};
		cmdBufferDesc.label = WGPUStringView("Graphics Command Buffer", WGPU_STRLEN);
		m_commandBuffer = m_encoder.finish(cmdBufferDesc);
		m_isRecording = false;
	}

	void *WebGPUGraphicsCommandList::GetNativeHandle() const
	{
		return static_cast<void *>(static_cast<WGPUCommandBuffer>(m_commandBuffer));
	}

	// WebGPU manages resource transitions internally — all barrier methods are noops.

	void WebGPUGraphicsCommandList::ResourceBarrier(std::span<const ResourceBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU handles resource state transitions automatically.
	}

	void WebGPUGraphicsCommandList::UAVBarrier(std::span<const UAVBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU handles UAV synchronization automatically.
	}

	void WebGPUGraphicsCommandList::BeginSplitBarrier(std::span<const SplitBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU does not support split barriers.
	}

	void WebGPUGraphicsCommandList::EndSplitBarrier(std::span<const SplitBarrierDescriptor> /*barriers*/)
	{
		// Noop: WebGPU does not support split barriers.
	}

	void WebGPUGraphicsCommandList::CopyBuffer(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsBuffer *dst,
											   uint64_t dstOffset, uint64_t size)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inRenderPass, "Cannot copy during render pass");
		HUSH_ASSERT(!m_inComputePass, "Cannot copy during compute pass");
		HUSH_ASSERT(src && dst, "Source and destination buffers must be valid");

		auto *srcBuffer = dynamic_cast<WebGPUBuffer *>(src);
		auto *dstBuffer = dynamic_cast<WebGPUBuffer *>(dst);

		m_encoder.copyBufferToBuffer(srcBuffer->GetBuffer(), srcOffset, dstBuffer->GetBuffer(), dstOffset, size);
	}

	void WebGPUGraphicsCommandList::CopyBufferToTexture(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsTexture *dst,
														uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width,
														uint32_t height, uint32_t depth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inRenderPass, "Cannot copy during render pass");
		HUSH_ASSERT(!m_inComputePass, "Cannot copy during compute pass");
		HUSH_ASSERT(src && dst, "Source buffer and destination texture must be valid");

		auto *srcBuffer = dynamic_cast<WebGPUBuffer *>(src);
		auto *dstTexture = dynamic_cast<WebGPUTexture *>(dst);

		wgpu::TexelCopyBufferInfo source{};
		source.buffer = srcBuffer->GetBuffer();
		source.layout.offset = srcOffset;
		source.layout.bytesPerRow = width * GetTextureBytesPerPixel(GetTextureFormat(dst));
		source.layout.rowsPerImage = height;

		wgpu::TexelCopyTextureInfo destination{};
		destination.texture = dstTexture->GetTexture();
		destination.mipLevel = 0;
		destination.origin = {.x = dstX, .y = dstY, .z = dstZ};
		destination.aspect = wgpu::TextureAspect::All;

		wgpu::Extent3D copySize{width, height, depth};

		m_encoder.copyBufferToTexture(source, destination, copySize);
	}

	void WebGPUGraphicsCommandList::CopyTextureToBuffer(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY,
														uint32_t srcZ, IGraphicsBuffer *dst, uint64_t dstOffset,
														uint32_t width, uint32_t height, uint32_t depth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inRenderPass, "Cannot copy during render pass");
		HUSH_ASSERT(!m_inComputePass, "Cannot copy during compute pass");
		HUSH_ASSERT(src && dst, "Source texture and destination buffer must be valid");

		auto *srcTexture = dynamic_cast<WebGPUTexture *>(src);
		auto *dstBuffer = dynamic_cast<WebGPUBuffer *>(dst);

		wgpu::TexelCopyTextureInfo source{};
		source.texture = srcTexture->GetTexture();
		source.mipLevel = 0;
		source.origin = {.x = srcX, .y = srcY, .z = srcZ};
		source.aspect = wgpu::TextureAspect::All;

		wgpu::TexelCopyBufferInfo destination{};
		destination.buffer = dstBuffer->GetBuffer();
		destination.layout.offset = dstOffset;
		destination.layout.bytesPerRow = width * GetTextureBytesPerPixel(GetTextureFormat(src));
		destination.layout.rowsPerImage = height;

		wgpu::Extent3D copySize{width, height, depth};

		m_encoder.copyTextureToBuffer(source, destination, copySize);
	}

	void WebGPUGraphicsCommandList::CopyTexture(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
												IGraphicsTexture *dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
												uint32_t width, uint32_t height, uint32_t depth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inRenderPass, "Cannot copy during render pass");
		HUSH_ASSERT(!m_inComputePass, "Cannot copy during compute pass");
		HUSH_ASSERT(src && dst, "Source and destination textures must be valid");

		auto *srcTexture = dynamic_cast<WebGPUTexture *>(src);
		auto *dstTexture = dynamic_cast<WebGPUTexture *>(dst);

		wgpu::TexelCopyTextureInfo source{};
		source.texture = srcTexture->GetTexture();
		source.mipLevel = 0;
		source.origin = {.x = srcX, .y = srcY, .z = srcZ};
		source.aspect = wgpu::TextureAspect::All;

		wgpu::TexelCopyTextureInfo destination{};
		destination.texture = dstTexture->GetTexture();
		destination.mipLevel = 0;
		destination.origin = {.x = dstX, .y = dstY, .z = dstZ};
		destination.aspect = wgpu::TextureAspect::All;

		wgpu::Extent3D copySize{width, height, depth};

		m_encoder.copyTextureToTexture(source, destination, copySize);
	}

	void WebGPUGraphicsCommandList::Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inRenderPass, "Cannot dispatch during render pass");

		// Begin compute pass if not already in one
		if (!m_inComputePass)
		{
			wgpu::ComputePassDescriptor computePassDesc{};
			computePassDesc.label = WGPUStringView("Compute Pass", WGPU_STRLEN);
			m_computePass = m_encoder.beginComputePass(computePassDesc);
			m_inComputePass = true;
		}

		m_computePass.dispatchWorkgroups(groupCountX, groupCountY, groupCountZ);
	}

	void WebGPUGraphicsCommandList::DispatchIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inRenderPass, "Cannot dispatch during render pass");
		HUSH_ASSERT(indirectArgsBuffer, "Indirect args buffer must be valid");

		auto *buffer = dynamic_cast<WebGPUBuffer *>(indirectArgsBuffer);

		// Begin compute pass if not already in one
		if (!m_inComputePass)
		{
			wgpu::ComputePassDescriptor computePassDesc{};
			computePassDesc.label = WGPUStringView("Compute Pass");
			m_computePass = m_encoder.beginComputePass(computePassDesc);
			m_inComputePass = true;
		}

		m_computePass.dispatchWorkgroupsIndirect(buffer->GetBuffer(), offset);
	}

	void WebGPUGraphicsCommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
										 uint32_t firstInstance)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to draw");

		m_renderPass.draw(vertexCount, instanceCount, firstVertex, firstInstance);
	}

	void WebGPUGraphicsCommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex,
												int32_t vertexOffset, uint32_t firstInstance)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to draw");

		m_renderPass.drawIndexed(indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
	}

	void WebGPUGraphicsCommandList::DrawIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to draw");
		HUSH_ASSERT(indirectArgsBuffer, "Indirect args buffer must be valid");

		auto *buffer = dynamic_cast<WebGPUBuffer *>(indirectArgsBuffer);
		m_renderPass.drawIndirect(buffer->GetBuffer(), offset);
	}

	void WebGPUGraphicsCommandList::DrawIndexedIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to draw");
		HUSH_ASSERT(indirectArgsBuffer, "Indirect args buffer must be valid");

		auto *buffer = dynamic_cast<WebGPUBuffer *>(indirectArgsBuffer);
		m_renderPass.drawIndexedIndirect(buffer->GetBuffer(), offset);
	}

	void WebGPUGraphicsCommandList::SetVertexBuffer(uint32_t slot, IGraphicsBuffer *buffer, uint64_t offset)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to set vertex buffer");
		HUSH_ASSERT(buffer, "Buffer must be valid");

		auto *vertexBuffer = dynamic_cast<WebGPUBuffer *>(buffer);
		m_renderPass.setVertexBuffer(slot, vertexBuffer->GetBuffer(), offset, vertexBuffer->GetSize() - offset);
	}

	void WebGPUGraphicsCommandList::SetIndexBuffer(IGraphicsBuffer *buffer, uint64_t offset)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to set index buffer");
		HUSH_ASSERT(buffer, "Buffer must be valid");

		auto *indexBuffer = dynamic_cast<WebGPUBuffer *>(buffer);
		// Assuming 32-bit indices; could be extended to support 16-bit
		m_renderPass.setIndexBuffer(indexBuffer->GetBuffer(), wgpu::IndexFormat::Uint32, offset,
									indexBuffer->GetSize() - offset);
	}

	void WebGPUGraphicsCommandList::SetViewport(float x, float y, float width, float height, float minDepth,
												float maxDepth)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to set viewport");

		m_renderPass.setViewport(x, y, width, height, minDepth, maxDepth);
	}

	void WebGPUGraphicsCommandList::SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to set scissor");

		// WebGPU scissor rect uses unsigned coordinates
		m_renderPass.setScissorRect(static_cast<uint32_t>(x), static_cast<uint32_t>(y), width, height);
	}

	void WebGPUGraphicsCommandList::BeginRenderPass(const RenderPassDescriptor &descriptor)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(!m_inRenderPass, "Already in a render pass");
		HUSH_ASSERT(!m_inComputePass, "Cannot begin render pass during compute pass");

		// Convert our descriptor to WebGPU descriptor
		wgpu::RenderPassDescriptor renderPassDesc{};
		if (!descriptor.debugLabel.empty())
		{
			renderPassDesc.label = WGPUStringView(descriptor.debugLabel.data(), descriptor.debugLabel.size());
		}

		// Color attachments
		std::array<wgpu::RenderPassColorAttachment, 8> colorAttachments{};
		for (uint32_t i = 0; i < descriptor.colorAttachmentCount; ++i)
		{
			const auto &colorAttach = descriptor.colorAttachments[i];
			auto *texture = dynamic_cast<WebGPUTexture *>(colorAttach.texture);

			colorAttachments[i].view = texture->GetView();

			// Load operation
			switch (colorAttach.loadOp)
			{
			case ELoadOp::Load:
				colorAttachments[i].loadOp = wgpu::LoadOp::Load;
				break;
			case ELoadOp::Clear:
				colorAttachments[i].loadOp = wgpu::LoadOp::Clear;
				colorAttachments[i].clearValue = {.r = colorAttach.clearValue.r,
												  .g = colorAttach.clearValue.g,
												  .b = colorAttach.clearValue.b,
												  .a = colorAttach.clearValue.a};
				break;
			case ELoadOp::DontCare:
				colorAttachments[i].loadOp = wgpu::LoadOp::Clear; // WebGPU doesn't have DontCare
				break;
			}

			// Store operation
			switch (colorAttach.storeOp)
			{
			case EStoreOp::Store:
				colorAttachments[i].storeOp = wgpu::StoreOp::Store;
				break;
			case EStoreOp::DontCare:
				colorAttachments[i].storeOp = wgpu::StoreOp::Discard;
				break;
			}

			// Resolve target (MSAA)
			if (colorAttach.resolveTarget != nullptr)
			{
				auto *resolveTexture = dynamic_cast<WebGPUTexture *>(colorAttach.resolveTarget);
				colorAttachments[i].resolveTarget = resolveTexture->GetView();
			}
		}

		renderPassDesc.colorAttachmentCount = descriptor.colorAttachmentCount;
		renderPassDesc.colorAttachments = colorAttachments.data();

		// Depth/stencil attachment
		wgpu::RenderPassDepthStencilAttachment depthStencilAttachment{};
		if (descriptor.depthStencilAttachment.has_value())
		{
			const auto &dsAttach = *descriptor.depthStencilAttachment;
			auto *texture = dynamic_cast<WebGPUTexture *>(dsAttach.texture);

			depthStencilAttachment.view = texture->GetView();

			// Depth operations
			switch (dsAttach.depthLoadOp)
			{
			case ELoadOp::Load:
				depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Load;
				break;
			case ELoadOp::Clear:
				depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
				depthStencilAttachment.depthClearValue = dsAttach.depthClearValue;
				break;
			case ELoadOp::DontCare:
				depthStencilAttachment.depthLoadOp = wgpu::LoadOp::Clear;
				break;
			}

			switch (dsAttach.depthStoreOp)
			{
			case EStoreOp::Store:
				depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Store;
				break;
			case EStoreOp::DontCare:
				depthStencilAttachment.depthStoreOp = wgpu::StoreOp::Discard;
				break;
			}

			depthStencilAttachment.depthReadOnly = static_cast<WGPUBool>(dsAttach.depthReadOnly);

			// Stencil operations
			switch (dsAttach.stencilLoadOp)
			{
			case ELoadOp::Load:
				depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Load;
				break;
			case ELoadOp::Clear:
				depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Clear;
				depthStencilAttachment.stencilClearValue = dsAttach.stencilClearValue;
				break;
			case ELoadOp::DontCare:
				depthStencilAttachment.stencilLoadOp = wgpu::LoadOp::Clear;
				break;
			}

			switch (dsAttach.stencilStoreOp)
			{
			case EStoreOp::Store:
				depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Store;
				break;
			case EStoreOp::DontCare:
				depthStencilAttachment.stencilStoreOp = wgpu::StoreOp::Discard;
				break;
			}

			depthStencilAttachment.stencilReadOnly = static_cast<WGPUBool>(dsAttach.stencilReadOnly);

			renderPassDesc.depthStencilAttachment = &depthStencilAttachment;
		}

		m_renderPass = m_encoder.beginRenderPass(renderPassDesc);
		m_inRenderPass = true;
	}

	void WebGPUGraphicsCommandList::EndRenderPass()
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Not in a render pass");

		m_renderPass.end();
		m_inRenderPass = false;
	}

	void WebGPUGraphicsCommandList::BindPipeline(IPipeline *pipeline)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to bind pipeline");
		HUSH_ASSERT(pipeline, "Pipeline must be valid");

		if (pipeline->IsGraphics())
		{
			auto *webgpuPipeline = dynamic_cast<WebGPUGraphicsPipeline *>(pipeline);
			HUSH_ASSERT(webgpuPipeline, "Pipeline must be a WebGPU graphics pipeline");
			m_renderPass.setPipeline(webgpuPipeline->GetPipeline());
		}
	}

	void WebGPUGraphicsCommandList::SetBindGroup(uint32_t groupIndex, IBindGroup *bindGroup,
												 std::span<const uint32_t> dynamicOffsets)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inRenderPass, "Must be in a render pass to set bind group");
		HUSH_ASSERT(bindGroup, "Bind group must be valid");

		auto *webgpuBindGroup = dynamic_cast<WebGPUBindGroup *>(bindGroup);
		HUSH_ASSERT(webgpuBindGroup, "Bind group must be a WebGPU bind group");
		m_renderPass.setBindGroup(groupIndex, webgpuBindGroup->GetBindGroup(), dynamicOffsets.size(),
								  dynamicOffsets.data());
	}

	void WebGPUGraphicsCommandList::BindComputePipeline(IComputePipeline *pipeline)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(pipeline, "Pipeline must be valid");

		// Begin compute pass if not already in one (end render pass first if needed)
		if (m_inRenderPass)
		{
			m_renderPass.end();
			m_renderPass = nullptr;
			m_inRenderPass = false;
		}

		if (!m_inComputePass)
		{
			wgpu::ComputePassDescriptor computePassDesc{};
			computePassDesc.label = WGPUStringView("Compute Pass");
			m_computePass = m_encoder.beginComputePass(computePassDesc);
			m_inComputePass = true;
		}

		auto *webgpuPipeline = dynamic_cast<WebGPUComputePipeline *>(pipeline);
		HUSH_ASSERT(webgpuPipeline, "Pipeline must be a WebGPU compute pipeline");
		m_computePass.setPipeline(webgpuPipeline->GetPipeline());
	}

	void WebGPUGraphicsCommandList::SetComputeBindGroup(uint32_t groupIndex, IBindGroup *bindGroup,
														std::span<const uint32_t> dynamicOffsets)
	{
		HUSH_ASSERT(m_isRecording, "Command list must be recording");
		HUSH_ASSERT(m_inComputePass, "Must be in a compute pass to set compute bind group");
		HUSH_ASSERT(bindGroup, "Bind group must be valid");

		auto *webgpuBindGroup = dynamic_cast<WebGPUBindGroup *>(bindGroup);
		HUSH_ASSERT(webgpuBindGroup, "Bind group must be a WebGPU bind group");
		m_computePass.setBindGroup(groupIndex, webgpuBindGroup->GetBindGroup(), dynamicOffsets.size(),
								   dynamicOffsets.data());
	}

} // namespace Hush::Graphics
