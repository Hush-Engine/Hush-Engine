#include "RenderGraphSystem.hpp"
#include "Components/RenderGraphBuilderComponent.hpp"
#include "Scene.hpp"

Hush::Graphics::RenderGraphSystem::RenderGraphSystem(Hush::Scene &scene, RenderGraph::RenderGraph *renderGraph)
	: ISystem(scene),
	  m_renderGraph(renderGraph)
{
}

void Hush::Graphics::RenderGraphSystem::Init()
{
	m_renderGraphBuilderQuery = GetScene().CreateQuery<RenderGraph::RenderGraphBuilderComponent>();
}

void Hush::Graphics::RenderGraphSystem::OnShutdown()
{
	// We don't need to do anything here.
}

void Hush::Graphics::RenderGraphSystem::OnUpdate([[maybe_unused]] float delta)
{
}

void Hush::Graphics::RenderGraphSystem::OnFixedUpdate([[maybe_unused]] float delta)
{
}

void Hush::Graphics::RenderGraphSystem::OnPreRender()
{
	if (m_renderGraph->IsDirty())
	{
		m_renderGraphBuilderQuery.Each([&](RenderGraph::RenderGraphBuilderComponent &builderComponent) {
			// We now that there is only one RenderGraphBuilderComponent, so we can just take the first one we find and
			// use it to build the graph.
			builderComponent.builderFunc(*m_renderGraph);
		});
		m_renderGraph->Compile();
	}
}

void Hush::Graphics::RenderGraphSystem::OnRender()
{
	m_renderGraph->Execute();
}

void Hush::Graphics::RenderGraphSystem::OnPostRender()
{
}
