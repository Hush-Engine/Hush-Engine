/*! \file RenderGraph.hpp
	\author Alan Ramirez Herrera
	\date 2025-11-17
	\brief RenderGraph implementation for rendering
*/

#pragma once

#include "RenderPassNode.hpp"
#include "ResourceId.hpp"
#include "ResourceNode.hpp"
#include "ResourceHandle.hpp"
#include <memory>
#include <string_view>
#include <cstdint>
#include <type_traits>

namespace Hush::RenderGraph
{
	// Forward-declare renderer context (expected to be provided by Renderer.hpp)
	class IRenderContext;

	class ICommandQueue;

	/// The type of pass to execute.
	/// Depending on the type, different command queues might be used. This
	/// depends on the underlying renderer implementation.
	enum class EPassType : uint8_t
	{
		/// Graphics pass
		Graphics,
		/// Compute pass
		Compute,
		/// Transfer pass
		Transfer,
	};

	// The render graph itself
	class RenderGraph
	{
		template <ResourceConcept T>
		ResourceId CreateResource(const std::string_view name, ResourceHandle::EHandleType type,
								  const typename T::Descriptor &descriptor, T &&resource)
		{
			const auto resourceId = static_cast<ResourceId>(m_resourceHandles.size());
			m_resourceHandles.emplace_back(ResourceHandle{descriptor, std::forward<T>(resource), type, resourceId});

			const auto resourceNodeId = static_cast<uint32_t>(m_resources.size());
			m_resources.emplace_back(
				ResourceNode{name, resourceNodeId, resourceId, ResourceHandle::RESOURCE_INITIAL_VERSION});

			return resourceNodeId;
		}

	public:
		// Context available during the build stage of a pass.
		// Allows creating resources and declaring usage (read/write).
		class BuildContext
		{
		public:
			BuildContext(RenderGraph &renderGraph, RenderPassNode &passNode);

			/// Create a resource inside of the pass
			/// @tparam T The type of resource to create
			/// @param name The name of the resource
			/// @param desc The descriptor for the resource
			/// @return The id of the created resource
			template <ResourceConcept T>
			ResourceId Create(std::string_view name, const typename T::Descriptor &&desc)
			{
				const ResourceId id =
					m_renderGraph.CreateResource(name, ResourceHandle::EHandleType::Transient, desc, T{});

				m_passNode.AddCreatedResource(id);
				return id;
			}

			/// Import a resource from outside of the render graph. For instance, swapchain images.
			/// @tparam T The type of resource to import
			/// @param name The name of the resource
			/// @param desc The descriptor for the resource
			/// @param resource The resource instance to import
			/// @return The id of the imported resource
			template <ResourceConcept T>
			ResourceId ImportResource(std::string_view name, const typename T::Descriptor &&desc, T &&resource)
			{
				const ResourceId id = m_renderGraph.CreateResource(name, ResourceHandle::EHandleType::External, desc,
																   std::forward<T>(resource));

				return id;
			}

			/// Read a resource inside of the pass
			/// @param id The resource to read
			/// @return The same resource id
			ResourceId Read(ResourceId id, uint32_t flags);

			/// Write a resource inside of the pass
			/// @param id The resource to write
			/// @return The same resource id
			ResourceId Write(ResourceId id, uint32_t flags);

			/// Set the culling mode for this pass.
			/// The two modes are:
			/// - CullIfPossible: The pass will be culled if it does not contribute to the final outputs.
			/// - NeverCull: The pass will never be culled.
			/// By default, all passes are set to CullIfPossible.
			void SetCullingMode(RenderPassNode::EPassCullingMode cullMode);

		private:
			RenderGraph &m_renderGraph;
			RenderPassNode &m_passNode;
		};
		RenderGraph() = default;

		// Add a pass with typed local data and the two callbacks (build, execute)
		template <typename PassData, typename BuildCallable, typename ExecuteCallable>
			requires(std::is_invocable_r_v<void, BuildCallable, BuildContext &, PassData &> &&
					 std::is_invocable_r_v<void, ExecuteCallable, PassData &, void *>)
		const PassData &AddPass(std::string_view name, BuildCallable &&buildCb, ExecuteCallable &&execCb,
								EPassType type)
		{
			auto pass = std::make_unique<Pass<PassData, decltype(execCb)>>(std::forward(execCb));

			auto &passData = pass->data;

			const auto passNodeId = static_cast<uint32_t>(m_passes.size());
			RenderPassNode &passNode = m_passes.emplace_back(name, passNodeId, std::forward(pass));

			BuildContext buildCtx(*this, passNode);

			std::invoke(std::forward<BuildCallable>(buildCb), buildCtx, passData);

			return passData;
		}

		/// Compile the render graph (optimize and prepare for execution).
		/// This step will eliminate passes that do not contribute to the final outputs.
		void Compile();

		/// Execute the render graph (invokes all execute callbacks).
		/// As part of the build step, some passes might have been eliminated.
		/// @note Depending on the underlying renderer implementation, we might record passes in
		/// parallel if possible.
		void Execute(void *ctx, void *allocator);

	private:
		IRenderContext *m_rctx = nullptr;
		std::vector<RenderPassNode> m_passes;
		std::vector<ResourceNode> m_resources;
		std::vector<ResourceHandle> m_resourceHandles;
	};
} // namespace Hush::RenderGraph
