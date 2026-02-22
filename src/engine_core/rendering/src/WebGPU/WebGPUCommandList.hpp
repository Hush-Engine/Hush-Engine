/*! \file WebGPUCommandList.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU implementation of command list interfaces
*/
#pragma once

#include "../RHI/ICommandList.hpp"
#include "../RHI/IBindGroup.hpp"
#include "../RHI/IPipeline.hpp"
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{
	class IPipeline;
	class IComputePipeline;
	class IBindGroup;

	/// @brief WebGPU implementation of copy command list
	class WebGPUCopyCommandList : public ICopyCommandList
	{
	public:
		explicit WebGPUCopyCommandList(wgpu::Device device);
		~WebGPUCopyCommandList() override;

		WebGPUCopyCommandList(const WebGPUCopyCommandList &) = delete;
		WebGPUCopyCommandList &operator=(const WebGPUCopyCommandList &) = delete;
		WebGPUCopyCommandList(WebGPUCopyCommandList &&) = delete;
		WebGPUCopyCommandList &operator=(WebGPUCopyCommandList &&) = delete;

		void Reset() override;
		void Close() override;
		[[nodiscard]]
		void *GetNativeHandle() const override;

		// Barrier methods (noop on WebGPU — transitions are managed internally by the API)
		void ResourceBarrier(std::span<const ResourceBarrierDescriptor> barriers) override;
		void UAVBarrier(std::span<const UAVBarrierDescriptor> barriers) override;
		void BeginSplitBarrier(std::span<const SplitBarrierDescriptor> barriers) override;
		void EndSplitBarrier(std::span<const SplitBarrierDescriptor> barriers) override;

		void CopyBuffer(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsBuffer *dst, uint64_t dstOffset,
						uint64_t size) override;

		void CopyBufferToTexture(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsTexture *dst, uint32_t dstX,
								 uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height, uint32_t depth,
								 uint32_t rowPitch) override;

		void CopyTextureToBuffer(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
								 IGraphicsBuffer *dst, uint64_t dstOffset, uint32_t width, uint32_t height,
								 uint32_t depth) override;

		void CopyTexture(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ, IGraphicsTexture *dst,
						 uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height,
						 uint32_t depth) override;

		[[nodiscard]]
		wgpu::CommandEncoder GetEncoder() const
		{
			return m_encoder;
		}
		[[nodiscard]]
		wgpu::CommandBuffer GetCommandBuffer() const
		{
			return m_commandBuffer;
		}

	protected:
		wgpu::Device m_device;
		wgpu::CommandEncoder m_encoder;
		wgpu::CommandBuffer m_commandBuffer;
		bool m_isRecording = false;
	};

	/// @brief WebGPU implementation of compute command list
	class WebGPUComputeCommandList : public IComputeCommandList
	{
	public:
		explicit WebGPUComputeCommandList(wgpu::Device device);
		~WebGPUComputeCommandList() override;

		WebGPUComputeCommandList(const WebGPUComputeCommandList &) = delete;
		WebGPUComputeCommandList &operator=(const WebGPUComputeCommandList &) = delete;
		WebGPUComputeCommandList(WebGPUComputeCommandList &&) = delete;
		WebGPUComputeCommandList &operator=(WebGPUComputeCommandList &&) = delete;

		void Reset() override;
		void Close() override;
		[[nodiscard]]
		void *GetNativeHandle() const override;

		// Barrier methods (noop on WebGPU — transitions are managed internally by the API)
		void ResourceBarrier(std::span<const ResourceBarrierDescriptor> barriers) override;
		void UAVBarrier(std::span<const UAVBarrierDescriptor> barriers) override;
		void BeginSplitBarrier(std::span<const SplitBarrierDescriptor> barriers) override;
		void EndSplitBarrier(std::span<const SplitBarrierDescriptor> barriers) override;

		void CopyBuffer(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsBuffer *dst, uint64_t dstOffset,
						uint64_t size) override;

		void CopyBufferToTexture(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsTexture *dst, uint32_t dstX,
								 uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height, uint32_t depth,
								 uint32_t rowPitch) override;

		void CopyTextureToBuffer(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
								 IGraphicsBuffer *dst, uint64_t dstOffset, uint32_t width, uint32_t height,
								 uint32_t depth) override;

		void CopyTexture(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ, IGraphicsTexture *dst,
						 uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height,
						 uint32_t depth) override;

		void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
		void DispatchIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset) override;

		void BindComputePipeline(IComputePipeline *pipeline) override;

		void SetComputeBindGroup(uint32_t groupIndex, IBindGroup *bindGroup,
								 std::span<const uint32_t> dynamicOffsets = {}) override;

		[[nodiscard]]
		wgpu::CommandEncoder GetEncoder() const
		{
			return m_encoder;
		}
		[[nodiscard]]
		wgpu::CommandBuffer GetCommandBuffer() const
		{
			return m_commandBuffer;
		}

	protected:
		wgpu::Device m_device;
		wgpu::CommandEncoder m_encoder;
		wgpu::CommandBuffer m_commandBuffer;
		wgpu::ComputePassEncoder m_computePass;
		bool m_isRecording = false;
		bool m_inComputePass = false;
	};

	/// @brief WebGPU implementation of graphics command list
	class WebGPUGraphicsCommandList : public IGraphicsCommandList
	{
	public:
		explicit WebGPUGraphicsCommandList(wgpu::Device device);
		~WebGPUGraphicsCommandList() override;

		WebGPUGraphicsCommandList(const WebGPUGraphicsCommandList &) = delete;
		WebGPUGraphicsCommandList &operator=(const WebGPUGraphicsCommandList &) = delete;
		WebGPUGraphicsCommandList(WebGPUGraphicsCommandList &&) = delete;
		WebGPUGraphicsCommandList &operator=(WebGPUGraphicsCommandList &&) = delete;

		void Reset() override;
		void Close() override;
		[[nodiscard]]
		void *GetNativeHandle() const override;

		// Barrier methods (noop on WebGPU — transitions are managed internally by the API)
		void ResourceBarrier(std::span<const ResourceBarrierDescriptor> barriers) override;
		void UAVBarrier(std::span<const UAVBarrierDescriptor> barriers) override;
		void BeginSplitBarrier(std::span<const SplitBarrierDescriptor> barriers) override;
		void EndSplitBarrier(std::span<const SplitBarrierDescriptor> barriers) override;

		// Copy commands
		void CopyBuffer(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsBuffer *dst, uint64_t dstOffset,
						uint64_t size) override;

		void CopyBufferToTexture(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsTexture *dst, uint32_t dstX,
								 uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height, uint32_t depth,
								 uint32_t rowPitch) override;

		void CopyTextureToBuffer(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
								 IGraphicsBuffer *dst, uint64_t dstOffset, uint32_t width, uint32_t height,
								 uint32_t depth) override;

		void CopyTexture(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ, IGraphicsTexture *dst,
						 uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height,
						 uint32_t depth) override;

		// Compute commands
		void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
		void DispatchIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset) override;

		void BindComputePipeline(IComputePipeline *pipeline) override;

		void SetComputeBindGroup(uint32_t groupIndex, IBindGroup *bindGroup,
								 std::span<const uint32_t> dynamicOffsets = {}) override;

		// Graphics commands
		void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;

		void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
						 uint32_t firstInstance) override;

		void DrawIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset) override;

		void DrawIndexedIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset) override;

		void SetVertexBuffer(uint32_t slot, IGraphicsBuffer *buffer, uint64_t offset = 0) override;

		void SetIndexBuffer(IGraphicsBuffer *buffer, uint64_t offset = 0) override;

		void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) override;

		void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) override;

		void BeginRenderPass(const RenderPassDescriptor &descriptor) override;

		void EndRenderPass() override;

		void BindPipeline(IPipeline *pipeline) override;

		void SetBindGroup(uint32_t groupIndex, IBindGroup *bindGroup,
						  std::span<const uint32_t> dynamicOffsets = {}) override;

		[[nodiscard]]
		wgpu::CommandEncoder GetEncoder() const
		{
			return m_encoder;
		}
		[[nodiscard]]
		wgpu::CommandBuffer GetCommandBuffer() const
		{
			return m_commandBuffer;
		}
		[[nodiscard]]
		wgpu::RenderPassEncoder GetRenderPass() const
		{
			return m_renderPass;
		}

	private:
		wgpu::Device m_device;
		wgpu::CommandEncoder m_encoder;
		wgpu::CommandBuffer m_commandBuffer;
		wgpu::RenderPassEncoder m_renderPass;
		wgpu::ComputePassEncoder m_computePass;
		bool m_isRecording = false;
		bool m_inRenderPass = false;
		bool m_inComputePass = false;
	};

} // namespace Hush::Graphics
