/*! \file RenderPass.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Render pass descriptor and related types for WebGPU-style render pass configuration
*/
#pragma once

#include "IGraphicsTexture.hpp"
#include <array>
#include <string_view>

namespace Hush::Graphics
{
	// ============================================================================
	// Render Pass Types
	// ============================================================================

	/// @brief Load operation for render pass attachments
	enum class ELoadOp
	{
		Load,      // Preserve existing contents
		Clear,     // Clear to a specified value
		DontCare,  // Don't care about existing contents (optimization hint)
	};

	/// @brief Store operation for render pass attachments
	enum class EStoreOp
	{
		Store,     // Store results to memory
		DontCare,  // Don't care about storing (optimization hint)
	};

	/// @brief Color clear value
	struct ClearColorValue
	{
		float r = 0.0f;
		float g = 0.0f;
		float b = 0.0f;
		float a = 0.0f;

		constexpr ClearColorValue() = default;
		constexpr ClearColorValue(float r, float g, float b, float a = 1.0f)
			: r(r), g(g), b(b), a(a) {}
	};

	/// @brief Depth/stencil clear value
	struct ClearDepthStencilValue
	{
		float depth = 1.0f;
		uint32_t stencil = 0;

		constexpr ClearDepthStencilValue() = default;
		constexpr ClearDepthStencilValue(float depth, uint32_t stencil = 0)
			: depth(depth), stencil(stencil) {}
	};

	/// @brief Color attachment descriptor for render passes
	struct RenderPassColorAttachment
	{
		IGraphicsTexture* texture = nullptr;
		uint32_t mipLevel = 0;
		uint32_t arrayLayer = 0;
		ELoadOp loadOp = ELoadOp::Clear;
		EStoreOp storeOp = EStoreOp::Store;
		ClearColorValue clearValue = {0.0f, 0.0f, 0.0f, 1.0f};

		// Optional resolve target for MSAA
		IGraphicsTexture* resolveTarget = nullptr;
	};

	/// @brief Depth/stencil attachment descriptor for render passes
	struct RenderPassDepthStencilAttachment
	{
		IGraphicsTexture* texture = nullptr;
		uint32_t mipLevel = 0;
		uint32_t arrayLayer = 0;

		ELoadOp depthLoadOp = ELoadOp::Clear;
		EStoreOp depthStoreOp = EStoreOp::Store;
		float depthClearValue = 1.0f;
		bool depthReadOnly = false;

		ELoadOp stencilLoadOp = ELoadOp::Clear;
		EStoreOp stencilStoreOp = EStoreOp::Store;
		uint32_t stencilClearValue = 0;
		bool stencilReadOnly = false;
	};

	/// @brief Render pass descriptor
	/// Describes the configuration for a render pass, including all attachments
	struct RenderPassDescriptor
	{
		// Color attachments
		std::array<RenderPassColorAttachment, 8> colorAttachments;
		uint32_t colorAttachmentCount = 0;

		// Optional depth/stencil attachment
		RenderPassDepthStencilAttachment* depthStencilAttachment = nullptr;

		// Debug label
		std::string_view debugLabel;

		/// @brief Helper to add a color attachment
		void AddColorAttachment(const RenderPassColorAttachment& attachment)
		{
			if (colorAttachmentCount < 8)
			{
				colorAttachments[colorAttachmentCount++] = attachment;
			}
		}

		/// @brief Helper to set depth/stencil attachment
		void SetDepthStencilAttachment(RenderPassDepthStencilAttachment* attachment)
		{
			depthStencilAttachment = attachment;
		}
	};

} // namespace Hush::Graphics
