/*! \file RenderGraphBuilderComponent.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief RenderGraph builder component for resource declaration and pass setup
*/

#pragma once

#include <functional>
#include "RenderGraph/RenderGraph.hpp"

namespace Hush::RenderGraph
{
	/// @brief Component that encapsulates a builder function for constructing the render graph.
	///
	/// When the RenderGraph is reset, it will invoke the builder function to allow the user to declare the render
	/// passes and resources for the new frame. This component is designed to be instantiated only once and persist
	/// across frames, as it holds the logic for building the graph structure.
	///
	/// An optional @c frameUpdateFunc can be provided to perform lightweight per-frame
	/// updates (e.g. swapping the swapchain backbuffer pointer via UpdateImport) without
	/// tearing down and recompiling the entire graph. When the graph is already compiled,
	/// only @c frameUpdateFunc is invoked; when the graph is dirty (first frame, after a
	/// resize, after an explicit reset, etc.), @c builderFunc is invoked to perform the
	/// full rebuild and the update callback is skipped for that frame since the builder
	/// already set up all resources.
	struct RenderGraphBuilderComponent
	{
		using BuilderFunc = std::function<void(RenderGraph &graph)>;

		/// @brief Function to build the render graph from scratch.
		///
		/// Called when the graph is dirty and needs a full rebuild (first frame,
		/// after a resize, after an explicit invalidation, etc.).  The callback
		/// should declare all passes and resources via AddPass / Create / Import.
		BuilderFunc builderFunc;

		/// @brief Optional lightweight per-frame update function.
		///
		/// Called every frame when the graph is already compiled and does NOT need
		/// a full rebuild.  Use this to update per-frame imported resources (e.g.
		/// swapchain backbuffer) via @c RenderGraph::UpdateImport without modifying
		/// graph topology or triggering recompilation.
		///
		/// If this is null, the system falls back to a full reset + rebuild +
		/// compile every frame (the previous behaviour).
		BuilderFunc frameUpdateFunc;
	};
} // namespace Hush::RenderGraph
