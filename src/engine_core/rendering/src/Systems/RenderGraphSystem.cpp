/*! \file RenderGraphSystem.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Render graph system implementation — full executor lifecycle management
*/
#include "RenderGraphSystem.hpp"
#include "Components/RenderGraphBuilderComponent.hpp"
#include "Logger.hpp"
#include "Scene.hpp"

Hush::Graphics::RenderGraphSystem::RenderGraphSystem(Hush::Scene &scene, RenderGraph::RenderDevice *renderDevice)
	: ISystem(scene),
	  m_renderDevice(renderDevice)
{
	HUSH_ASSERT(m_renderDevice != nullptr, "RenderGraphSystem requires a valid RenderDevice!");
}

void Hush::Graphics::RenderGraphSystem::Init()
{
	m_renderGraphBuilderQuery = GetScene().CreateQuery<RenderGraph::RenderGraphBuilderComponent>();
}

void Hush::Graphics::RenderGraphSystem::OnShutdown()
{
	// If a frame is still active (e.g. early shutdown), try to end it cleanly
	// so the swapchain / GPU state is not left dangling.
	if (m_frameActive)
	{
		m_renderDevice->EndFrame();
		m_frameActive = false;
	}
}

void Hush::Graphics::RenderGraphSystem::OnUpdate([[maybe_unused]] float delta)
{
}

void Hush::Graphics::RenderGraphSystem::OnFixedUpdate([[maybe_unused]] float delta)
{
}

void Hush::Graphics::RenderGraphSystem::OnPreRender()
{
	m_renderDevice->BeginFrame();
	m_frameActive = true;

	auto &renderGraph = m_renderDevice->GetRenderGraph();

	bool canTakeFastPath = renderGraph.IsCompiled();

	if (canTakeFastPath)
	{
		// Verify every builder provides a frameUpdateFunc.  If any builder
		// is missing one, we must fall back to the full rebuild so the
		// graph stays correct.
		m_renderGraphBuilderQuery.Each([&canTakeFastPath](RenderGraph::RenderGraphBuilderComponent &builderComponent) {
			if (!builderComponent.frameUpdateFunc)
			{
				canTakeFastPath = false;
			}
		});
	}

	if (canTakeFastPath)
	{
		// --------------------------------------------------------------
		// FAST PATH — graph topology is unchanged, skip recompilation.
		// --------------------------------------------------------------

		// Reset only the executor's per-frame state (fence counters,
		// resource state tracker). Graph passes and compilation are kept.
		m_renderDevice->SoftReset();

		// Let each builder update per-frame imported resources in-place.
		m_renderGraphBuilderQuery.Each([&renderGraph](RenderGraph::RenderGraphBuilderComponent &builderComponent) {
			builderComponent.frameUpdateFunc(renderGraph);
		});
	}
	else
	{
		// Clear the previous frame's render graph (passes, resources,
		// compilation state) and reset the executor's per-frame state.
		// Fence objects themselves are kept alive and reused across frames.
		m_renderDevice->Reset();

		// Iterate every RenderGraphBuilderComponent entity and invoke its
		// builder function.  Each builder declares render passes and
		// resources via the RenderGraph's AddPass / BuildContext API.
		m_renderGraphBuilderQuery.Each([&renderGraph](RenderGraph::RenderGraphBuilderComponent &builderComponent) {
			if (builderComponent.builderFunc)
			{
				builderComponent.builderFunc(renderGraph);
			}
		});

		// Topological sort, dependency level assignment, and SSIS-based
		// synchronization point culling.  If the graph is somehow already
		// compiled (no builder added any passes, or a builder called
		// Compile() itself), Compile() is a no-op.
		m_renderDevice->Compile();
	}
}

void Hush::Graphics::RenderGraphSystem::OnRender()
{
	if (!m_renderDevice->IsCompiled())
	{
		Hush::LogWarn("RenderGraphSystem::OnRender — graph is not compiled, "
					  "skipping execution.");
		return;
	}

	m_renderDevice->Execute();
}

void Hush::Graphics::RenderGraphSystem::OnPostRender()
{
	if (m_frameActive)
	{
		m_renderDevice->EndFrame();
		m_frameActive = false;
	}
}
