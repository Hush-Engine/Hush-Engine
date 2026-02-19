/*! \file RenderGraphBuilderComponent.hpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief RenderGraph builder component for resource declaration and pass setup
*/

#pragma once

#include <functional>
#include "RenderGraph/RenderGraph.hpp"

namespace Hush::RenderGraph
{
    /// @brief Component that encapsulates a builder function for constructing the render graph.
    ///
    /// When the RenderGraph is reset, it will invoke the builder function to allow the user to declare the render passes and resources for the new frame.
    /// This component is designed to be instantiated only once and persist across frames, as it holds the logic for building the graph structure.
    struct RenderGraphBuilderComponent
    {
        using BuilderFunc = std::function<void(RenderGraph& graph)>;

        /// @brief Function to build the render graph. This should be set by the user to define the graph structure.
        BuilderFunc builderFunc;
    };
}
