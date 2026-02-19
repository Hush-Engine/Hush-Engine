/*! \file ICommandList.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Command list interfaces for graphics abstraction
*/
#pragma once

#include "IGraphicsBuffer.hpp"
#include "IGraphicsTexture.hpp"
#include "RenderPass.hpp"

namespace Hush::Graphics
{
	class IPipeline;

	/// @brief Base interface for command lists, which are used to record GPU commands for execution.
	class ICommandList
	{
	public:
		ICommandList() = default;
		virtual ~ICommandList() = default;

		ICommandList(const ICommandList &) = delete;
		ICommandList &operator=(const ICommandList &) = delete;
		ICommandList(ICommandList &&) = delete;
		ICommandList &operator=(ICommandList &&) = delete;

		/// @brief Reset command list for recording
		virtual void Reset() = 0;

		/// @brief Close command list after recording
		virtual void Close() = 0;

		/// @brief Get native handle (API-specific)
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};

	/// @brief Command list interface for copy/transfer operations
	class ICopyCommandList : public ICommandList
	{
	public:
		ICopyCommandList() = default;
		~ICopyCommandList() override = default;

		ICopyCommandList(const ICopyCommandList &) = delete;
		ICopyCommandList &operator=(const ICopyCommandList &) = delete;
		ICopyCommandList(ICopyCommandList &&) = delete;
		ICopyCommandList &operator=(ICopyCommandList &&) = delete;

		/// @brief Record a buffer-to-buffer copy command
		virtual void CopyBuffer(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsBuffer *dst, uint64_t dstOffset,
								uint64_t size) = 0;

		/// @brief Record a buffer-to-texture copy command
		virtual void CopyBufferToTexture(IGraphicsBuffer *src, uint64_t srcOffset, IGraphicsTexture *dst, uint32_t dstX,
										 uint32_t dstY, uint32_t dstZ, uint32_t width, uint32_t height,
										 uint32_t depth) = 0;

		/// @brief Record a texture-to-buffer copy command
		virtual void CopyTextureToBuffer(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
										 IGraphicsBuffer *dst, uint64_t dstOffset, uint32_t width, uint32_t height,
										 uint32_t depth) = 0;

		/// @brief Record a texture-to-texture copy command
		virtual void CopyTexture(IGraphicsTexture *src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
								 IGraphicsTexture *dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t width,
								 uint32_t height, uint32_t depth) = 0;
	};

	/// @brief Command list interface for compute operations.
	///
	/// Compute command lists can also record copy commands.
	class IComputeCommandList : public ICopyCommandList
	{
	public:
		using ICopyCommandList::ICopyCommandList;

		IComputeCommandList() = default;
		~IComputeCommandList() override = default;

		IComputeCommandList(const IComputeCommandList &) = delete;
		IComputeCommandList &operator=(const IComputeCommandList &) = delete;
		IComputeCommandList(IComputeCommandList &&) = delete;
		IComputeCommandList &operator=(IComputeCommandList &&) = delete;

		/// @brief Record a compute dispatch command
		virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

		/// @brief Record a compute dispatch indirect command
		virtual void DispatchIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset) = 0;

		// TODO: Finish IComputeCommandList.
	};

	/// @brief Command list interface for graphics operations.
	///
	/// This is the most feature-rich command list type, supporting all graphics commands as well as compute and copy
	/// commands.
	class IGraphicsCommandList : public IComputeCommandList
	{
	public:
		using IComputeCommandList::IComputeCommandList;

		IGraphicsCommandList() = default;
		~IGraphicsCommandList() override = default;

		IGraphicsCommandList(const IGraphicsCommandList &) = delete;
		IGraphicsCommandList &operator=(const IGraphicsCommandList &) = delete;
		IGraphicsCommandList(IGraphicsCommandList &&) = delete;
		IGraphicsCommandList &operator=(IGraphicsCommandList &&) = delete;

		/// @brief Record a draw command
		///
		/// @param vertexCount Number of vertices to draw
		/// @param instanceCount Number of instances to draw
		/// @param firstVertex Index of the first vertex
		/// @param firstInstance Index of the first instance
		virtual void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex,
						  uint32_t firstInstance) = 0;

		/// @brief Record an indexed draw command
		///
		/// @param indexCount Number of indices to draw
		/// @param instanceCount Number of instances to draw
		/// @param firstIndex Index of the first index
		/// @param vertexOffset Value added to each index before fetching vertex data
		/// @param firstInstance Index of the first instance
		virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
								 uint32_t firstInstance) = 0;

		/// @brief Record an indirect draw command
		/// @param indirectArgsBuffer Buffer containing draw arguments (must be created with indirect usage flag)
		/// @param offset Byte offset into the buffer where draw arguments start
		virtual void DrawIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset) = 0;

		/// @brief Record an indexed indirect draw command
		///
		/// @param indirectArgsBuffer Buffer containing draw arguments (must be created with indirect usage flag)
		/// @param offset Byte offset into the buffer where draw arguments start
		virtual void DrawIndexedIndirect(IGraphicsBuffer *indirectArgsBuffer, uint64_t offset) = 0;

		/// @brief Set vertex buffer
		/// @param slot Binding slot for the vertex buffer
		/// @param buffer Vertex buffer to bind
		/// @param offset Byte offset into the buffer
		virtual void SetVertexBuffer(uint32_t slot, IGraphicsBuffer *buffer, uint64_t offset = 0) = 0;

		/// @brief Set index buffer
		///
		/// @param buffer Index buffer to bind
		/// @param offset Byte offset into the buffer
		virtual void SetIndexBuffer(IGraphicsBuffer *buffer, uint64_t offset = 0) = 0;

		/// @brief Set viewport
		///
		/// @param x Top-left x coordinate of the viewport
		/// @param y Top-left y coordinate of the viewport
		/// @param width Width of the viewport
		/// @param height Height of the viewport
		/// @param minDepth Minimum depth value (0.0 to 1.0)
		/// @param maxDepth Maximum depth value (0.0 to 1.0)
		virtual void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) = 0;

		/// @brief Set scissor rectangle
		///
		/// @param x Top-left x coordinate of the scissor rectangle
		/// @param y Top-left y coordinate of the scissor rectangle
		/// @param width Width of the scissor rectangle
		/// @param height Height of the scissor rectangle
		virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

		/// @brief Begin render pass with descriptor (WebGPU-style)
		///
		/// @param descriptor Render pass configuration describing color and depth/stencil attachments
		virtual void BeginRenderPass(const RenderPassDescriptor &descriptor) = 0;

		/// @brief End render pass
		virtual void EndRenderPass() = 0;

		/// @brief Bind graphics pipeline
		///
		/// @param pipeline Pipeline to bind (must be a graphics pipeline compatible with the current render pass)
		virtual void BindPipeline(IPipeline *pipeline) = 0;
	};

} // namespace Hush::Graphics
