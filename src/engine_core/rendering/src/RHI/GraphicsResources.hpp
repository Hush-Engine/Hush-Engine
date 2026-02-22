/*! \file GraphicsResources.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Graphics resource types for RenderGraph integration

	This file defines concrete resource types that bridge the RHI and RenderGraph systems.
	These types implement the ResourceConcept for use with RenderGraph's BuildContext.
*/
#pragma once

#include "GraphicsTypes.hpp"
#include "IBindGroup.hpp"
#include "IGraphicsBuffer.hpp"
#include "IGraphicsTexture.hpp"
#include "ISampler.hpp"
#include "IShaderModule.hpp"
#include "IPipeline.hpp"
#include "PipelineDescriptor.hpp"
#include "RenderPass.hpp"
#include <memory>
#include <vector>

namespace Hush::Graphics
{
	class IGraphicsDevice;
	class IGraphicsCommandList;
	class IComputeCommandList;
	class ICopyCommandList;

	// =========================================================================
	// Texture resources
	// =========================================================================

	/// @brief Texture resource descriptor for RenderGraph
	/// This is used during BuildContext to declare texture creation parameters
	struct TextureResource
	{
		using Descriptor = TextureDescriptor;

		/// @brief Runtime texture handle (set during execution)
		std::unique_ptr<IGraphicsTexture> texture;

		/// @brief Resource descriptor
		TextureDescriptor descriptor;

		/// @brief Get the underlying texture (convenience accessor)
		[[nodiscard]]
		IGraphicsTexture *Get() const
		{
			return texture.get();
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return texture != nullptr;
		}

		void CreateResource(const TextureDescriptor &textureDescriptor, IGraphicsDevice *device);

		void DestroyResource(const TextureDescriptor &textureDescriptor, IGraphicsDevice *device);
	};

	/// @brief Import/external texture resource for RenderGraph
	/// Used when you want to reference an existing texture created outside the graph
	struct ImportedTextureResource
	{
		using Descriptor = TextureDescriptor;

		/// @brief Imported texture handle
		IGraphicsTexture *texture = nullptr;

		/// @brief Get the underlying texture (convenience accessor)
		[[nodiscard]]
		IGraphicsTexture *Get() const
		{
			return texture;
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return texture != nullptr;
		}

		void CreateResource([[maybe_unused]] const TextureDescriptor &textureDescriptor,
							[[maybe_unused]] IGraphicsDevice *device)
		{
		}

		void DestroyResource([[maybe_unused]] const TextureDescriptor &textureDescriptor,
							 [[maybe_unused]] IGraphicsDevice *device)
		{
		}
	};

	// =========================================================================
	// Buffer resources
	// =========================================================================

	/// @brief Buffer resource descriptor for RenderGraph
	/// This is used during BuildContext to declare buffer creation parameters
	struct BufferResource
	{
		using Descriptor = BufferDescriptor;

		/// @brief Runtime buffer handle (set during execution)
		std::shared_ptr<IGraphicsBuffer> buffer;

		/// @brief Resource descriptor
		BufferDescriptor descriptor;

		/// @brief Get the underlying buffer (convenience accessor)
		[[nodiscard]]
		IGraphicsBuffer *Get() const
		{
			return buffer.get();
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return buffer != nullptr;
		}

		void CreateResource(const BufferDescriptor &bufferDescriptor, IGraphicsDevice *device);

		void DestroyResource(const BufferDescriptor &bufferDescriptor, IGraphicsDevice *device);
	};

	/// @brief Import/external buffer resource for RenderGraph
	/// Used when you want to reference an existing buffer created outside the graph
	struct ImportedBufferResource
	{
		using Descriptor = BufferDescriptor;

		/// @brief Imported buffer handle
		IGraphicsBuffer *buffer = nullptr;

		/// @brief Get the underlying buffer (convenience accessor)
		[[nodiscard]]
		IGraphicsBuffer *Get() const
		{
			return buffer;
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return buffer != nullptr;
		}

		void CreateResource([[maybe_unused]] const BufferDescriptor &bufferDescriptor,
							[[maybe_unused]] IGraphicsDevice *device)
		{
		}

		void DestroyResource([[maybe_unused]] const BufferDescriptor &bufferDescriptor,
							 [[maybe_unused]] IGraphicsDevice *device)
		{
		}
	};

	/// @brief Shader module resource for RenderGraph
	/// This is used during BuildContext to declare shader module creation parameters.
	/// Owns the shader module lifetime so client code does not need to manage it.
	struct ShaderResource
	{
		using Descriptor = ShaderModuleDescriptor;

		/// @brief Runtime shader module handle (set during execution)
		std::unique_ptr<IShaderModule> shaderModule;

		/// @brief Resource descriptor
		ShaderModuleDescriptor descriptor;

		/// @brief Get the underlying shader module (convenience accessor)
		[[nodiscard]]
		IShaderModule *Get() const
		{
			return shaderModule.get();
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return shaderModule != nullptr && shaderModule->IsValid();
		}

		void CreateResource(const ShaderModuleDescriptor &shaderDescriptor, IGraphicsDevice *device);

		void DestroyResource(const ShaderModuleDescriptor &shaderDescriptor, IGraphicsDevice *device);
	};

	/// @brief Import/external shader module resource for RenderGraph
	/// Used when you want to reference an existing shader module created outside the graph
	struct ImportedShaderResource
	{
		using Descriptor = ShaderModuleDescriptor;

		/// @brief Imported shader module handle (non-owning)
		IShaderModule *shaderModule = nullptr;

		/// @brief Get the underlying shader module (convenience accessor)
		[[nodiscard]]
		IShaderModule *Get() const
		{
			return shaderModule;
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return shaderModule != nullptr && shaderModule->IsValid();
		}

		void CreateResource([[maybe_unused]] const ShaderModuleDescriptor &shaderDescriptor,
							[[maybe_unused]] IGraphicsDevice *device)
		{
		}

		void DestroyResource([[maybe_unused]] const ShaderModuleDescriptor &shaderDescriptor,
							 [[maybe_unused]] IGraphicsDevice *device)
		{
		}
	};

	// =========================================================================
	// Bind group layout resources
	// =========================================================================

	/// @brief Bind group layout resource for RenderGraph
	/// Owns the bind group layout lifetime so client code does not need to manage it.
	struct BindGroupLayoutResource
	{
		using Descriptor = BindGroupLayoutDescriptor;

		/// @brief Runtime bind group layout handle
		std::unique_ptr<IBindGroupLayout> layout;

		/// @brief Resource descriptor
		BindGroupLayoutDescriptor descriptor;

		/// @brief Get the underlying bind group layout (convenience accessor)
		[[nodiscard]]
		IBindGroupLayout *Get() const
		{
			return layout.get();
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return layout != nullptr && layout->IsValid();
		}

		void CreateResource(const BindGroupLayoutDescriptor &layoutDescriptor, IGraphicsDevice *device);

		void DestroyResource(const BindGroupLayoutDescriptor &layoutDescriptor, IGraphicsDevice *device);
	};

	/// @brief Import/external bind group layout resource for RenderGraph
	/// Used when you want to reference an existing bind group layout created outside the graph
	struct ImportedBindGroupLayoutResource
	{
		using Descriptor = BindGroupLayoutDescriptor;

		/// @brief Imported bind group layout handle (non-owning)
		IBindGroupLayout *layout = nullptr;

		/// @brief Get the underlying bind group layout (convenience accessor)
		[[nodiscard]]
		IBindGroupLayout *Get() const
		{
			return layout;
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return layout != nullptr && layout->IsValid();
		}

		void CreateResource([[maybe_unused]] const BindGroupLayoutDescriptor &layoutDescriptor,
							[[maybe_unused]] IGraphicsDevice *device)
		{
		}

		void DestroyResource([[maybe_unused]] const BindGroupLayoutDescriptor &layoutDescriptor,
							 [[maybe_unused]] IGraphicsDevice *device)
		{
		}
	};

	/// @brief Bind group resource for RenderGraph
	/// Owns the bind group lifetime so client code does not need to manage it.
	struct BindGroupResource
	{
		using Descriptor = BindGroupDescriptor;

		/// @brief Runtime bind group handle
		std::unique_ptr<IBindGroup> bindGroup;

		/// @brief Resource descriptor
		BindGroupDescriptor descriptor;

		/// @brief Get the underlying bind group (convenience accessor)
		[[nodiscard]]
		IBindGroup *Get() const
		{
			return bindGroup.get();
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return bindGroup != nullptr && bindGroup->IsValid();
		}

		void CreateResource(const BindGroupDescriptor &bindGroupDescriptor, IGraphicsDevice *device);

		void DestroyResource(const BindGroupDescriptor &bindGroupDescriptor, IGraphicsDevice *device);
	};

	/// @brief Import/external bind group resource for RenderGraph
	/// Used when you want to reference an existing bind group created outside the graph
	struct ImportedBindGroupResource
	{
		using Descriptor = BindGroupDescriptor;

		/// @brief Imported bind group handle (non-owning)
		IBindGroup *bindGroup = nullptr;

		/// @brief Get the underlying bind group (convenience accessor)
		[[nodiscard]]
		IBindGroup *Get() const
		{
			return bindGroup;
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return bindGroup != nullptr && bindGroup->IsValid();
		}

		void CreateResource([[maybe_unused]] const BindGroupDescriptor &bindGroupDescriptor,
							[[maybe_unused]] IGraphicsDevice *device)
		{
		}

		void DestroyResource([[maybe_unused]] const BindGroupDescriptor &bindGroupDescriptor,
							 [[maybe_unused]] IGraphicsDevice *device)
		{
		}
	};

	// =========================================================================
	// Sampler resources
	// =========================================================================

	/// @brief Sampler resource for RenderGraph
	/// Owns the sampler lifetime so client code does not need to manage it.
	struct SamplerResource
	{
		using Descriptor = SamplerDescriptor;

		/// @brief Runtime sampler handle
		std::unique_ptr<ISampler> sampler;

		/// @brief Resource descriptor
		SamplerDescriptor descriptor;

		/// @brief Get the underlying sampler (convenience accessor)
		[[nodiscard]]
		ISampler *Get() const
		{
			return sampler.get();
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return sampler != nullptr;
		}

		void CreateResource(const SamplerDescriptor &samplerDescriptor, IGraphicsDevice *device);

		void DestroyResource(const SamplerDescriptor &samplerDescriptor, IGraphicsDevice *device);
	};

	/// @brief Import/external sampler resource for RenderGraph
	/// Used when you want to reference an existing sampler created outside the graph
	struct ImportedSamplerResource
	{
		using Descriptor = SamplerDescriptor;

		/// @brief Imported sampler handle (non-owning)
		ISampler *sampler = nullptr;

		/// @brief Get the underlying sampler (convenience accessor)
		[[nodiscard]]
		ISampler *Get() const
		{
			return sampler;
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return sampler != nullptr;
		}

		void CreateResource([[maybe_unused]] const SamplerDescriptor &samplerDescriptor,
							[[maybe_unused]] IGraphicsDevice *device)
		{
		}

		void DestroyResource([[maybe_unused]] const SamplerDescriptor &samplerDescriptor,
							 [[maybe_unused]] IGraphicsDevice *device)
		{
		}
	};

	// =========================================================================
	// Graphics pipeline resources
	// =========================================================================

	/// @brief Graphics pipeline resource for RenderGraph
	/// Owns the graphics pipeline lifetime so client code does not need to manage it.
	struct GraphicsPipelineResource
	{
		using Descriptor = GraphicsPipelineDescriptor;

		/// @brief Runtime graphics pipeline handle
		std::unique_ptr<IGraphicsPipeline> pipeline;

		/// @brief Resource descriptor
		GraphicsPipelineDescriptor descriptor;

		/// @brief Get the underlying graphics pipeline (convenience accessor)
		[[nodiscard]]
		IGraphicsPipeline *Get() const
		{
			return pipeline.get();
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return pipeline != nullptr && pipeline->IsValid();
		}

		void CreateResource(const GraphicsPipelineDescriptor &pipelineDescriptor, IGraphicsDevice *device);

		void DestroyResource(const GraphicsPipelineDescriptor &pipelineDescriptor, IGraphicsDevice *device);
	};

	/// @brief Import/external graphics pipeline resource for RenderGraph
	/// Used when you want to reference an existing graphics pipeline created outside the graph
	struct ImportedGraphicsPipelineResource
	{
		using Descriptor = GraphicsPipelineDescriptor;

		/// @brief Imported graphics pipeline handle (non-owning)
		IGraphicsPipeline *pipeline = nullptr;

		/// @brief Get the underlying graphics pipeline (convenience accessor)
		[[nodiscard]]
		IGraphicsPipeline *Get() const
		{
			return pipeline;
		}

		/// @brief Check if the resource is valid
		[[nodiscard]]
		bool IsValid() const
		{
			return pipeline != nullptr && pipeline->IsValid();
		}

		void CreateResource([[maybe_unused]] const GraphicsPipelineDescriptor &pipelineDescriptor,
							[[maybe_unused]] IGraphicsDevice *device)
		{
		}

		void DestroyResource([[maybe_unused]] const GraphicsPipelineDescriptor &pipelineDescriptor,
							 [[maybe_unused]] IGraphicsDevice *device)
		{
		}
	};

	// =========================================================================
	// Render execution context
	// =========================================================================

	/// @brief Render execution context provided to passes during Execute phase
	/// This interface provides access to command lists and resource resolution
	class IRenderContext
	{
	public:
		virtual ~IRenderContext() = default;

		IRenderContext(const IRenderContext &) = delete;
		IRenderContext &operator=(const IRenderContext &) = delete;
		IRenderContext(IRenderContext &&) = delete;
		IRenderContext &operator=(IRenderContext &&) = delete;

		/// @brief Get graphics command list for the current pass
		[[nodiscard]]
		virtual IGraphicsCommandList *GetGraphicsCommandList() = 0;

		/// @brief Get compute command list for the current pass
		[[nodiscard]]
		virtual IComputeCommandList *GetComputeCommandList() = 0;

		/// @brief Get transfer/copy command list for the current pass
		[[nodiscard]]
		virtual ICopyCommandList *GetCopyCommandList() = 0;

		/// @brief Resolve a resource ID to a texture
		/// @param resourceId Resource ID from the render graph
		/// @return Texture pointer, or nullptr if not found or wrong type
		[[nodiscard]]
		virtual IGraphicsTexture *GetTexture(uint32_t resourceId) = 0;

		/// @brief Resolve a resource ID to a buffer
		/// @param resourceId Resource ID from the render graph
		/// @return Buffer pointer, or nullptr if not found or wrong type
		[[nodiscard]]
		virtual IGraphicsBuffer *GetBuffer(uint32_t resourceId) = 0;

		/// @brief Get the graphics device for resource creation or queries
		[[nodiscard]]
		virtual IGraphicsDevice *GetDevice() = 0;

		/// @brief Get current frame index (for double/triple buffering)
		[[nodiscard]]
		virtual uint32_t GetFrameIndex() const = 0;

		/// @brief Get swapchain texture for presenting (if available)
		[[nodiscard]]
		virtual IGraphicsTexture *GetSwapchainTexture() = 0;
	};

	// =========================================================================
	// Common pass data structures
	// =========================================================================

	/// @brief Common data for a simple graphics pass with color and depth
	struct GraphicsPassData
	{
		/// @brief Color attachment resource ID
		uint32_t colorTarget = 0;

		/// @brief Depth/stencil attachment resource ID (optional)
		uint32_t depthTarget = 0;

		/// @brief Clear color value
		ClearColorValue clearColor = {0.0f, 0.0f, 0.0f, 1.0f};

		/// @brief Clear depth value
		float clearDepth = 1.0f;

		/// @brief Load operation for color attachment
		ELoadOp colorLoadOp = ELoadOp::Clear;

		/// @brief Load operation for depth attachment
		ELoadOp depthLoadOp = ELoadOp::Clear;

		/// @brief Helper to build a RenderPassDescriptor from this data
		[[nodiscard]]
		RenderPassDescriptor BuildDescriptor(IRenderContext *ctx) const
		{
			RenderPassDescriptor desc{};

			// Setup color attachment
			if (colorTarget != 0)
			{
				RenderPassColorAttachment color{};
				color.texture = ctx->GetTexture(colorTarget);
				color.loadOp = colorLoadOp;
				color.storeOp = EStoreOp::Store;
				color.clearValue = clearColor;
				desc.AddColorAttachment(color);
			}

			// Setup depth attachment
			if (depthTarget != 0)
			{
				RenderPassDepthStencilAttachment depth{};
				depth.texture = ctx->GetTexture(depthTarget);
				depth.depthLoadOp = depthLoadOp;
				depth.depthStoreOp = EStoreOp::Store;
				depth.depthClearValue = clearDepth;
				depth.depthReadOnly = false;
				desc.SetDepthStencilAttachment(depth);
			}

			return desc;
		}
	};

	/// @brief Common data for a compute pass
	struct ComputePassData
	{
		/// @brief Input buffer resource IDs
		std::vector<uint32_t> inputBuffers;

		/// @brief Output buffer resource IDs
		std::vector<uint32_t> outputBuffers;

		/// @brief Input texture resource IDs
		std::vector<uint32_t> inputTextures;

		/// @brief Output texture resource IDs
		std::vector<uint32_t> outputTextures;

		/// @brief Dispatch dimensions
		uint32_t groupCountX = 1;
		uint32_t groupCountY = 1;
		uint32_t groupCountZ = 1;
	};

	/// @brief Common data for a transfer/copy pass
	struct TransferPassData
	{
		/// @brief Source resource ID
		uint32_t sourceResource = 0;

		/// @brief Destination resource ID
		uint32_t destinationResource = 0;

		/// @brief Copy region information (buffer offset or texture coordinates)
		struct CopyRegion
		{
			uint64_t srcOffset = 0;
			uint64_t dstOffset = 0;
			uint64_t size = 0;
		} region;
	};

	/// @brief Dummy resource type for render graph resources that don't correspond to actual GPU resources.
	///
	/// This can be used to specify dependencies between passes in the render graph without needing a real texture or
	/// buffer (e.g. for synchronization or logical grouping).
	///
	/// For example, a pass that performs a compute operation without any actual GPU resources could use a DummyResource
	/// as its output, and subsequent passes could declare dependencies on that output to ensure correct execution
	/// order.
	struct DummyResource
	{
		struct Descriptor
		{
			// No actual descriptor fields needed for a dummy resource
		};
		void CreateResource([[maybe_unused]] const Descriptor &descriptor, [[maybe_unused]] IGraphicsDevice *device)
		{
			// No actual resource to create
		}

		void DestroyResource([[maybe_unused]] const Descriptor &descriptor, [[maybe_unused]] IGraphicsDevice *device)
		{
			// No actual resource to destroy
		}
	};

} // namespace Hush::Graphics
