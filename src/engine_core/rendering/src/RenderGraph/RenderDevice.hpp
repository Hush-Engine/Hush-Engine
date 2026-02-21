/*! \file RenderDevice.hpp
	\author Alan Ramirez Herrera
	\date 2025-11-17
	\brief RenderDevice — high-level orchestrator that owns the graphics device,
		   render graph, and executor.
*/
#pragma once

#include <string_view>
#include <type_traits>
#include <utility>
#include "RenderGraph.hpp"
#include "RenderGraphExecutor.hpp"
#include "RHI/IGraphicsDevice.hpp"

namespace Hush::RenderGraph
{
	/// High-level rendering orchestrator.
	///
	/// Owns the render graph (declaration + compilation) and the executor
	/// (runtime GPU work), and holds a non-owning reference to the graphics
	/// device that both depend on.
	///
	/// This is the class that game/engine code should interact with for
	/// all render-graph-based rendering. Lower-level access to the device
	/// or graph internals is available through accessor methods when needed.
	class RenderDevice
	{
	public:
		/// Construct a RenderDevice from a non-owning device pointer.
		///
		/// @param device The graphics device backend. Must outlive this
		///               RenderDevice instance. Ownership is NOT transferred.
		explicit RenderDevice(Hush::Graphics::IGraphicsDevice *device)
			: m_device(device),
			  m_executor(device)
		{
			// Populate the queue map so the render graph maps logical pass
			// types to the physical queue indices reported by this device.
			// On multi-queue backends (D3D12, Vulkan) this is the identity
			// mapping.  On single-queue backends (WebGPU) every pass type
			// collapses to queue 0, which eliminates all cross-queue sync.
			m_graph.SetQueueMap(EPassType::Graphics,
								device->MapPassTypeToQueueIndex(Hush::Graphics::EQueueType::Graphics));
			m_graph.SetQueueMap(EPassType::Compute,
								device->MapPassTypeToQueueIndex(Hush::Graphics::EQueueType::Compute));
			m_graph.SetQueueMap(EPassType::Transfer,
								device->MapPassTypeToQueueIndex(Hush::Graphics::EQueueType::Transfer));
		}

		~RenderDevice() = default;

		RenderDevice(const RenderDevice &) = delete;
		RenderDevice &operator=(const RenderDevice &) = delete;
		RenderDevice(RenderDevice &&) = delete;
		RenderDevice &operator=(RenderDevice &&) = delete;

		/// Begin a new frame.
		///
		/// Delegates to the graphics device's BeginFrame() to acquire the next
		/// swapchain image. Call this before registering any passes for the frame.
		void BeginFrame()
		{
			m_device->BeginFrame();
		}

		/// End the current frame and present.
		///
		/// Delegates to the graphics device's EndFrame() which typically presents
		/// the swapchain image. Call this after execution is complete.
		void EndFrame()
		{
			m_device->EndFrame();
		}

		/// Register a render pass with typed local data, a build callback, and
		/// an execute callback.
		///
		/// This is a thin forwarding wrapper around RenderGraph::AddPass. See
		/// that method for full documentation on the callback signatures.
		///
		/// @tparam PassData  Per-pass data struct (default-constructible).
		/// @tparam BuildFn   void(BuildContext&, PassData&)
		/// @tparam ExecuteFn void(PassData&, ICommandList*, ResourceManager&)
		/// @param passType   The queue type this pass targets.
		/// @param name       Debug name for the pass.
		/// @param buildFn    Called immediately to declare resource dependencies.
		/// @param execFn     Called during execution to record GPU commands.
		/// @return Reference to the initialized PassData (owned by the graph).
		template <typename PassData, typename BuildFn, typename ExecuteFn>
			requires(
				std::is_invocable_r_v<void, BuildFn, RenderGraph::BuildContext &, PassData &> &&
				std::is_invocable_r_v<void, ExecuteFn, PassData &, Hush::Graphics::ICommandList *, ResourceManager &>)
		const PassData &AddPass(EPassType passType, std::string_view name, BuildFn &&buildFn, ExecuteFn &&execFn)
		{
			return m_graph.AddPass<PassData>(passType, name, std::forward<BuildFn>(buildFn),
											 std::forward<ExecuteFn>(execFn));
		}

		/// Compile the render graph if it is dirty.
		///
		/// This performs topological sort, dependency level assignment, and
		/// SSIS-based synchronization point culling. It is a no-op if the
		/// graph is already compiled.
		void Compile()
		{
			if (m_graph.IsDirty())
			{
				m_graph.Compile();
			}
		}

		/// Execute the compiled render graph.
		///
		/// This:
		///   1. Realizes any unrealized transient GPU resources.
		///   2. Initializes resource state tracking.
		///   3. For each dependency level, builds barriers, batches command
		///      lists, and submits them with proper fence synchronization.
		///
		/// @pre The graph must be compiled (not dirty). Call Compile() first
		///      or use CompileAndExecute().
		void Execute()
		{
			m_executor.Execute(m_graph);
		}

		/// Convenience method: compile (if dirty) then execute.
		///
		/// This is the typical call in a frame loop after all passes have
		/// been registered.
		void CompileAndExecute()
		{
			Compile();
			Execute();
		}

		/// Reset the render graph and executor state for a fresh frame.
		///
		/// Clears all passes, resources, and compilation state from the graph.
		/// Resets the executor's per-frame fence counters and resource state
		/// tracker (fence objects themselves are reused).
		///
		/// Call this at the start of a frame (before or after BeginFrame) if
		/// you are rebuilding the graph each frame. If your graph is static
		/// across frames, you only need to call this when the graph changes.
		void Reset()
		{
			m_graph.Reset();
			m_executor.ResetFrameState();
		}

		/// Lightweight per-frame reset that keeps the graph compiled.
		///
		/// Only resets the executor's per-frame state (resource state tracker
		/// and fence value counters). The graph's passes, resources, topology,
		/// and compilation output are all preserved.
		///
		/// Use this instead of Reset() when the graph topology has not changed
		/// and you only need to update per-frame imported resources (e.g.
		/// swapchain backbuffer) via RenderGraph::UpdateImport().
		void SoftReset()
		{
			m_executor.ResetFrameState();
		}

		/// Mark the render graph as dirty, forcing a full rebuild next frame.
		///
		/// Unlike Reset(), this does NOT clear passes, resources, or compilation
		/// data immediately. It simply flips the graph's state flag so that the
		/// next frame's OnPreRender() takes the slow path (full Reset + rebuild
		/// + Compile).
		///
		/// Call this when something external invalidates the current graph
		/// structure — e.g. a window resize that changes render target
		/// dimensions, a scene change that adds/removes passes, etc.
		void Invalidate() noexcept
		{
			m_graph.Invalidate();
		}

		/// Get a mutable reference to the underlying render graph.
		///
		/// Useful for advanced scenarios where you need direct access to the
		/// graph structure (e.g. for debugging, visualization, or custom
		/// compilation steps).
		[[nodiscard]]
		RenderGraph &GetRenderGraph() noexcept
		{
			return m_graph;
		}

		/// Get a const reference to the underlying render graph.
		[[nodiscard]]
		const RenderGraph &GetRenderGraph() const noexcept
		{
			return m_graph;
		}

		/// Get a mutable reference to the executor.
		///
		/// Useful for advanced scenarios like manually controlling fence
		/// values or inspecting execution state.
		[[nodiscard]]
		RenderGraphExecutor &GetExecutor() noexcept
		{
			return m_executor;
		}

		/// Get a const reference to the executor.
		[[nodiscard]]
		const RenderGraphExecutor &GetExecutor() const noexcept
		{
			return m_executor;
		}

		/// Get the underlying graphics device.
		///
		/// Useful for creating resources outside the graph (e.g. swapchain
		/// images that are later imported via BuildContext::Import).
		[[nodiscard]]
		Hush::Graphics::IGraphicsDevice *GetGraphicsDevice() const noexcept
		{
			return m_device;
		}

		/// Get the resource manager from the render graph.
		///
		/// Convenience shortcut for GetRenderGraph().GetResourceManager().
		[[nodiscard]]
		ResourceManager &GetResourceManager() noexcept
		{
			return m_graph.GetResourceManager();
		}

		/// Get the resource manager from the render graph (const).
		[[nodiscard]]
		const ResourceManager &GetResourceManager() const noexcept
		{
			return m_graph.GetResourceManager();
		}

		/// Check if the render graph is compiled and ready for execution.
		[[nodiscard]]
		bool IsCompiled() const noexcept
		{
			return m_graph.IsCompiled();
		}

		/// Check if the render graph is dirty and needs recompilation.
		[[nodiscard]]
		bool IsDirty() const noexcept
		{
			return m_graph.IsDirty();
		}

		/// Resize the swapchain through the graphics device.
		///
		/// @param width  New width in pixels.
		/// @param height New height in pixels.
		void Resize(uint32_t width, uint32_t height)
		{
			m_device->Resize(width, height);
		}

	private:
		/// Non-owning pointer to the graphics device backend.
		/// Must outlive this RenderDevice.
		Hush::Graphics::IGraphicsDevice *m_device = nullptr;

		/// The render graph — pure dependency graph of passes and resources.
		/// Built by the user via AddPass(), compiled via Compile().
		RenderGraph m_graph;

		/// The executor — runtime GPU execution engine.
		/// Takes a compiled graph and handles fences, barriers, batching,
		/// command list recording, and submission.
		RenderGraphExecutor m_executor;
	};

} // namespace Hush::RenderGraph
