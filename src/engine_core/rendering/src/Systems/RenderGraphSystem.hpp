/*! \file RenderGraphSystem.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Render graph system for managing render passes, resources, and
		   executor lifecycle.
*/

#pragma once

#include "Components/RenderGraphBuilderComponent.hpp"
#include "Query.hpp"
#include "RenderGraph/RenderDevice.hpp"
#include "RenderGraph/RenderGraphExecutor.hpp"
#include "ISystem.hpp"

namespace Hush::Graphics
{
	class RenderGraphSystem : public ISystem
	{
	public:
		/// @brief Construct the system with a scene and the render device.
		///
		/// @param scene        The scene this system belongs to.
		/// @param renderDevice Non-owning pointer to the render device that owns
		///                     the graph, executor, and graphics device reference.
		///                     Must outlive this system.
		RenderGraphSystem(Scene &scene, RenderGraph::RenderDevice *renderDevice);
		~RenderGraphSystem() override = default;

		RenderGraphSystem(const RenderGraphSystem &) = delete;
		RenderGraphSystem &operator=(const RenderGraphSystem &) = delete;
		RenderGraphSystem(RenderGraphSystem &&) = delete;
		RenderGraphSystem &operator=(RenderGraphSystem &&) = delete;

		void Init() override;
		void OnShutdown() override;
		void OnUpdate(float delta) override;
		void OnFixedUpdate(float delta) override;

		/// @brief Begin the frame and prepare the graph for execution.
		///
		/// Takes one of two paths:
		///
		/// **Fast path** (graph already compiled, all builders have frameUpdateFunc):
		///   1. BeginFrame — acquires the next swapchain image.
		///   2. SoftReset — resets only the executor's per-frame state (fence
		///      counters, resource state tracker). Graph topology is preserved.
		///   3. Per-frame update — invokes each builder's frameUpdateFunc to
		///      update imported resources (e.g. swapchain backbuffer) via
		///      RenderGraph::UpdateImport(). No recompilation needed.
		///
		/// **Slow path** (first frame, after Invalidate(), or missing frameUpdateFunc):
		///   1. BeginFrame — acquires the next swapchain image.
		///   2. Reset — clears graph passes, resources, compilation state AND
		///      resets the executor's per-frame state.
		///   3. Rebuild — iterates all RenderGraphBuilderComponent entities and
		///      invokes their builderFunc to declare passes and resources.
		///   4. Compile — topological sort, dependency levels, SSIS culling.
		void OnPreRender() override;

		/// @brief Execute the compiled render graph.
		///
		/// Delegates to RenderGraphExecutor::Execute() which:
		///   - Realizes any unrealized transient GPU resources.
		///   - Initializes per-resource state tracking from imported initial states.
		///   - For each dependency level: detects multi-queue reads, selects the
		///     most competent queue for transition rerouting, computes resource
		///     barriers, builds command list batches bounded by fence waits/signals,
		///     records pass work into command lists, and submits batches.
		void OnRender() override;

		/// @brief End the frame and present the swapchain image.
		void OnPostRender() override;

		[[nodiscard]]
		std::string_view GetName() const override
		{
			return "RenderGraphSystem";
		}

		/// @brief Get the render device (graph + executor + device reference).
		[[nodiscard]]
		Hush::RenderGraph::RenderDevice *GetRenderDevice() const noexcept
		{
			return m_renderDevice;
		}

		/// @brief Convenience: get the render graph executor.
		///
		/// Useful for inspecting executor state (fence values, resource state
		/// tracker) during debugging or for advanced manual control.
		[[nodiscard]]
		Hush::RenderGraph::RenderGraphExecutor &GetExecutor() noexcept
		{
			return m_renderDevice->GetExecutor();
		}

		/// @brief Convenience: get the render graph executor (const).
		[[nodiscard]]
		const Hush::RenderGraph::RenderGraphExecutor &GetExecutor() const noexcept
		{
			return m_renderDevice->GetExecutor();
		}

		/// @brief Convenience: get the render graph.
		[[nodiscard]]
		Hush::RenderGraph::RenderGraph &GetRenderGraph() noexcept
		{
			return m_renderDevice->GetRenderGraph();
		}

		/// @brief Convenience: get the render graph (const).
		[[nodiscard]]
		const Hush::RenderGraph::RenderGraph &GetRenderGraph() const noexcept
		{
			return m_renderDevice->GetRenderGraph();
		}

		/// @brief Convenience: get the underlying graphics device.
		[[nodiscard]]
		IGraphicsDevice *GetGraphicsDevice() const noexcept
		{
			return m_renderDevice->GetGraphicsDevice();
		}

		/// @brief Check whether the render graph has been compiled and is ready
		///        for execution this frame.
		[[nodiscard]]
		bool IsGraphCompiled() const noexcept
		{
			return m_renderDevice->IsCompiled();
		}

		/// @brief Returns true if BeginFrame() has been called for the current
		///        frame and EndFrame() has not yet been called.
		[[nodiscard]]
		bool IsFrameActive() const noexcept
		{
			return m_frameActive;
		}

	private:
		/// Non-owning pointer to the render device that owns the graph, executor,
		/// and graphics device reference. Must outlive this system.
		Hush::RenderGraph::RenderDevice *m_renderDevice = nullptr;

		/// ECS query for RenderGraphBuilderComponent entities.
		Hush::Query<Hush::RenderGraph::RenderGraphBuilderComponent> m_renderGraphBuilderQuery;

		/// Tracks whether we are between BeginFrame and EndFrame for this frame.
		/// Used to guard against double-begin or end-without-begin.
		bool m_frameActive = false;
	};
} // namespace Hush::Graphics
