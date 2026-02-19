/*! \file RenderGraphSystem.hpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief Render graph system for managing render passes and resources
*/

#pragma once

#include "Components/RenderGraphBuilderComponent.hpp"
#include "Query.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "ISystem.hpp"

namespace Hush::Graphics
{
	class RenderGraphSystem : public ISystem
	{
	public:
		RenderGraphSystem(Scene &scene, RenderGraph::RenderGraph *renderGraph);
		~RenderGraphSystem() override = default;

		RenderGraphSystem(const RenderGraphSystem &) = delete;
		RenderGraphSystem &operator=(const RenderGraphSystem &) = delete;
		RenderGraphSystem(RenderGraphSystem &&) = delete;
		RenderGraphSystem &operator=(RenderGraphSystem &&) = delete;

		void Init() override;
		void OnShutdown() override;
		void OnUpdate(float delta) override;
		void OnFixedUpdate(float delta) override;
		void OnRender() override;
		void OnPreRender() override;
		void OnPostRender() override;

		[[nodiscard]]
		std::string_view GetName() const override
		{
			return "RenderGraphSystem";
		}

	private:
		Hush::RenderGraph::RenderGraph *m_renderGraph;
		Hush::Query<Hush::RenderGraph::RenderGraphBuilderComponent> m_renderGraphBuilderQuery;
	};
} // namespace Hush::Graphics
