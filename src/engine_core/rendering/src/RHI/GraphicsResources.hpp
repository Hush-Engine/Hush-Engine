/*! \file GraphicsResources.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Graphics resource types for RenderGraph integration

	This file defines concrete resource types that bridge the RHI and RenderGraph systems.
	These types implement the ResourceConcept for use with RenderGraph's BuildContext.
*/
#pragma once

#include "GraphicsTypes.hpp"
#include "IGraphicsBuffer.hpp"
#include "IGraphicsTexture.hpp"
#include "RenderPass.hpp"
#include <memory>
#include <vector>

namespace Hush::Graphics
{
	class IGraphicsDevice;
	class IGraphicsCommandList;
	class IComputeCommandList;
	class ICopyCommandList;

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
		[[nodiscard]] IGraphicsTexture* Get() const { return texture.get(); }

		/// @brief Check if the resource is valid
		[[nodiscard]] bool IsValid() const { return texture != nullptr; }

		void CreateResource(const TextureDescriptor& textureDescriptor, IGraphicsDevice* device);

		void DestroyResource(const TextureDescriptor& textureDescriptor, IGraphicsDevice* device);
	};

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
		[[nodiscard]] IGraphicsBuffer* Get() const { return buffer.get(); }

		/// @brief Check if the resource is valid
		[[nodiscard]] bool IsValid() const { return buffer != nullptr; }
	};

	/// @brief Import/external texture resource for RenderGraph
	/// Used when you want to reference an existing texture created outside the graph
	struct ImportedTextureResource
	{
		using Descriptor = TextureDescriptor;

		/// @brief Imported texture handle
		IGraphicsTexture* texture = nullptr;

		/// @brief Get the underlying texture (convenience accessor)
		[[nodiscard]] IGraphicsTexture* Get() const { return texture; }

		/// @brief Check if the resource is valid
		[[nodiscard]] bool IsValid() const { return texture != nullptr; }

		void CreateResource([[maybe_unused]] const TextureDescriptor& textureDescriptor, [[maybe_unused]] IGraphicsDevice* device)
		{
		}

		void DestroyResource([[maybe_unused]] const TextureDescriptor& textureDescriptor, [[maybe_unused]] IGraphicsDevice* device)
		{
		}
	};

	/// @brief Import/external buffer resource for RenderGraph
	/// Used when you want to reference an existing buffer created outside the graph
	struct ImportedBufferResource
	{
		using Descriptor = BufferDescriptor;

		/// @brief Imported buffer handle
		IGraphicsBuffer* buffer = nullptr;

		/// @brief Get the underlying buffer (convenience accessor)
		[[nodiscard]] IGraphicsBuffer* Get() const { return buffer; }

		/// @brief Check if the resource is valid
		[[nodiscard]] bool IsValid() const { return buffer != nullptr; }
	};

	// ============================================================================
	// Render Context Interface for RenderGraph Execution
	// ============================================================================

	/// @brief Render execution context provided to passes during Execute phase
	/// This interface provides access to command lists and resource resolution
	class IRenderContext
	{
	public:
		virtual ~IRenderContext() = default;

		IRenderContext(const IRenderContext&) = delete;
		IRenderContext& operator=(const IRenderContext&) = delete;
		IRenderContext(IRenderContext&&) = delete;
		IRenderContext& operator=(IRenderContext&&) = delete;

		// ========================================================================
		// Command List Access
		// ========================================================================

		/// @brief Get graphics command list for the current pass
		[[nodiscard]] virtual IGraphicsCommandList* GetGraphicsCommandList() = 0;

		/// @brief Get compute command list for the current pass
		[[nodiscard]] virtual IComputeCommandList* GetComputeCommandList() = 0;

		/// @brief Get transfer/copy command list for the current pass
		[[nodiscard]] virtual ICopyCommandList* GetCopyCommandList() = 0;

		// ========================================================================
		// Resource Resolution
		// ========================================================================

		/// @brief Resolve a resource ID to a texture
		/// @param resourceId Resource ID from the render graph
		/// @return Texture pointer, or nullptr if not found or wrong type
		[[nodiscard]] virtual IGraphicsTexture* GetTexture(uint32_t resourceId) = 0;

		/// @brief Resolve a resource ID to a buffer
		/// @param resourceId Resource ID from the render graph
		/// @return Buffer pointer, or nullptr if not found or wrong type
		[[nodiscard]] virtual IGraphicsBuffer* GetBuffer(uint32_t resourceId) = 0;

		// ========================================================================
		// Device Access
		// ========================================================================

		/// @brief Get the graphics device for resource creation or queries
		[[nodiscard]] virtual IGraphicsDevice* GetDevice() = 0;

		// ========================================================================
		// Frame Information
		// ========================================================================

		/// @brief Get current frame index (for double/triple buffering)
		[[nodiscard]] virtual uint32_t GetFrameIndex() const = 0;

		/// @brief Get swapchain texture for presenting (if available)
		[[nodiscard]] virtual IGraphicsTexture* GetSwapchainTexture() = 0;
	};

	// ============================================================================
	// Helper Structures for Common Pass Data Patterns
	// ============================================================================

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
		[[nodiscard]] RenderPassDescriptor BuildDescriptor(IRenderContext* ctx) const
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
				static RenderPassDepthStencilAttachment depth{};
				depth.texture = ctx->GetTexture(depthTarget);
				depth.depthLoadOp = depthLoadOp;
				depth.depthStoreOp = EStoreOp::Store;
				depth.depthClearValue = clearDepth;
				depth.depthReadOnly = false;
				desc.SetDepthStencilAttachment(&depth);
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

} // namespace Hush::Graphics
