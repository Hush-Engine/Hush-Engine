/*! \file FrameResources.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-20
	\brief Frame resources for the passes of the default renderer.
*/

#pragma once

#include "RenderGraph/RenderGraph.hpp"
#include "WindowRenderer.hpp"
#include "Scene.hpp"

namespace Hush::Graphics::DefaultRenderer
{
	struct GeometryPassResources
	{
		Hush::RenderGraph::ResourceId geometryBufferAlbedo{};
		Hush::RenderGraph::ResourceId geometryBufferNormal{};
		Hush::RenderGraph::ResourceId geometryDepthBuffer{};
	};

	struct LightingPassResources
	{
		Hush::RenderGraph::ResourceId lightingBuffer{};
	};

	/// @brief Resources for the final pass that copies the rendered image to the backbuffer.
	struct FinalPassResources
	{
		Hush::RenderGraph::ResourceId backbuffer{};
	};

	// void AddDefaultRenderer(Hush::WindowRenderer &windowRenderer, Hush::Scene *scene);
} // namespace Hush::Graphics::DefaultRenderer
