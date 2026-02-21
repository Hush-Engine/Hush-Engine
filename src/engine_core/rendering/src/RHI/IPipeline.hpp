/*! \file IPipeline.hpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief Abstract pipeline interfaces for graphics and compute pipelines.
*/
#pragma once

#include <cstdint>
#include <string_view>

namespace Hush::Graphics
{
	/// @brief Discriminator for the kind of pipeline (graphics vs compute).
	enum class EPipelineType : uint32_t
	{
		Graphics = 0,
		Compute,
	};

	/// @brief Abstract base interface for all GPU pipeline state objects.
	///
	/// This is the type accepted by IGraphicsCommandList::BindPipeline().
	/// Callers can query the pipeline type to determine whether it is a
	/// graphics or compute pipeline, and down-cast if needed.
	class IPipeline
	{
	public:
		IPipeline() = default;
		virtual ~IPipeline() = default;

		IPipeline(const IPipeline &) = delete;
		IPipeline &operator=(const IPipeline &) = delete;
		IPipeline(IPipeline &&) = delete;
		IPipeline &operator=(IPipeline &&) = delete;

		/// @brief Get the type of this pipeline (graphics or compute).
		[[nodiscard]]
		virtual EPipelineType GetType() const = 0;

		/// @brief Convenience: returns true if this is a graphics pipeline.
		[[nodiscard]]
		bool IsGraphics() const
		{
			return GetType() == EPipelineType::Graphics;
		}

		/// @brief Convenience: returns true if this is a compute pipeline.
		[[nodiscard]]
		bool IsCompute() const
		{
			return GetType() == EPipelineType::Compute;
		}

		/// @brief Check if the pipeline was created successfully and is usable.
		///
		/// A pipeline that failed creation (e.g. shader compilation error,
		/// incompatible state) will return false.
		[[nodiscard]]
		virtual bool IsValid() const = 0;

		/// @brief Get the debug name assigned at creation time.
		///
		/// Returns an empty string_view if no debug name was provided.
		[[nodiscard]]
		virtual std::string_view GetDebugName() const = 0;

		/// @brief Get the native API handle for the pipeline.
		///
		/// The returned pointer's type depends on the backend:
		///   - WebGPU graphics: wgpu::RenderPipeline*
		///   - WebGPU compute:  wgpu::ComputePipeline*
		///   - Vulkan:          VkPipeline*
		///   - D3D12:           ID3D12PipelineState*
		///
		/// Ownership is NOT transferred; the pipeline object retains ownership.
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};

	/// @brief Interface for a compiled graphics (rasterization) pipeline.
	///
	/// A graphics pipeline combines:
	///   - Vertex and fragment shader stages
	///   - Vertex input layout (buffer bindings + attribute descriptions)
	///   - Primitive topology and rasterizer state
	///   - Blend state per color target
	///   - Depth/stencil state
	///   - Multisample state
	///   - Bind group layouts (resource binding signature)
	///
	/// Once created, the pipeline is immutable.  To change state you must
	/// create a new pipeline.  Pipeline creation can be expensive, so
	/// applications should cache and reuse pipelines across frames.
	class IGraphicsPipeline : public IPipeline
	{
	public:
		IGraphicsPipeline() = default;
		~IGraphicsPipeline() override = default;

		IGraphicsPipeline(const IGraphicsPipeline &) = delete;
		IGraphicsPipeline &operator=(const IGraphicsPipeline &) = delete;
		IGraphicsPipeline(IGraphicsPipeline &&) = delete;
		IGraphicsPipeline &operator=(IGraphicsPipeline &&) = delete;

		/// @brief Always returns EPipelineType::Graphics.
		[[nodiscard]]
		EPipelineType GetType() const override
		{
			return EPipelineType::Graphics;
		}

		/// @brief Get the number of color targets this pipeline was created with.
		///
		/// This corresponds to the number of ColorTargetState entries in the
		/// GraphicsPipelineDescriptor used at creation time.  The render pass
		/// that this pipeline is used with must have a matching number of
		/// color attachments.
		[[nodiscard]]
		virtual uint32_t GetColorTargetCount() const = 0;

		/// @brief Returns true if the pipeline was created with depth/stencil
		///        state enabled.
		///
		/// When true, the render pass must include a depth/stencil attachment.
		[[nodiscard]]
		virtual bool HasDepthStencil() const = 0;

		/// @brief Get the number of vertex buffer slots this pipeline expects.
		///
		/// Corresponds to the number of VertexBufferLayout entries in the
		/// descriptor.  Before a draw call, the application must bind at least
		/// this many vertex buffers via SetVertexBuffer().
		[[nodiscard]]
		virtual uint32_t GetVertexBufferSlotCount() const = 0;
	};

	/// @brief Interface for a compiled compute pipeline.
	///
	/// A compute pipeline combines:
	///   - A single compute shader stage
	///   - Bind group layouts (resource binding signature)
	///
	/// Compute pipelines are bound before Dispatch() / DispatchIndirect() calls
	/// on a compute or graphics command list.
	class IComputePipeline : public IPipeline
	{
	public:
		IComputePipeline() = default;
		~IComputePipeline() override = default;

		IComputePipeline(const IComputePipeline &) = delete;
		IComputePipeline &operator=(const IComputePipeline &) = delete;
		IComputePipeline(IComputePipeline &&) = delete;
		IComputePipeline &operator=(IComputePipeline &&) = delete;

		/// @brief Always returns EPipelineType::Compute.
		[[nodiscard]]
		EPipelineType GetType() const override
		{
			return EPipelineType::Compute;
		}
	};

} // namespace Hush::Graphics
