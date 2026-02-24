/*! \file RenderGraphExecutor.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-18
	\brief RenderGraph executor — all GPU execution infrastructure.
*/
#pragma once

#include <vector>
#include <memory>
#include <limits>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include "RHI/IGraphicsDevice.hpp"
#include "RHI/IFence.hpp"
#include "RHI/ICommandList.hpp"
#include "RHI/ICommandQueue.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "RenderGraph.hpp"

namespace Hush::RenderGraph
{
	/// Tracks the current GPU state of a resource across passes and queues.
	///
	/// The executor maintains one of these per resource. It is used during
	/// execution to compute resource barriers:
	///   - Which state a resource is currently in.
	///   - Which queue last accessed it (needed for cross-queue transitions).
	///   - Whether the resource is currently being read by multiple queues
	///     simultaneously (multi-queue read).
	struct ResourceStateEntry
	{
		Hush::Graphics::EResourceState currentState = Hush::Graphics::EResourceState::Undefined;

		/// Index of the queue that last wrote or transitioned this resource.
		/// UINT32_MAX means "unknown / never accessed."
		uint32_t owningQueueIndex = std::numeric_limits<uint32_t>::max();

		/// True when the resource is being read by more than one queue in the
		/// same dependency level. In that case barriers must be rerouted.
		bool inMultiQueueRead = false;
	};

	/// Manages per-resource GPU state for the duration of a frame's execution.
	///
	/// Initialized from the graph's imported resource initial states and reset
	/// each frame. The executor updates entries as it inserts barriers.
	class ResourceStateTracker
	{
	public:
		ResourceStateTracker() = default;
		~ResourceStateTracker() = default;

		ResourceStateTracker(const ResourceStateTracker &) = delete;
		ResourceStateTracker &operator=(const ResourceStateTracker &) = delete;
		ResourceStateTracker(ResourceStateTracker &&) = default;
		ResourceStateTracker &operator=(ResourceStateTracker &&) = default;

		/// Initialize state tracking from a compiled graph.
		///
		/// Creates entries for every resource in the graph. Imported resources
		/// get their initial state from the graph's stored initial-state map;
		/// transient resources start as Undefined.
		void InitializeFromGraph(const RenderGraph &graph)
		{
			m_states.clear();

			const auto &initialStates = graph.GetImportedResourceInitialStates();

			graph.GetResourceManager().ForEachResource([&](ResourceId id, [[maybe_unused]]
																		  const ResourceHandle &handle) {
				ResourceStateEntry entry;
				auto it = initialStates.find(id);
				if (it != initialStates.end())
				{
					entry.currentState = it->second;
				}
				m_states.emplace(id, entry);
			});
		}

		/// Get the current tracked state of a resource.
		[[nodiscard]]
		ResourceStateEntry *GetState(const ResourceId &id)
		{
			auto it = m_states.find(id);
			return (it != m_states.end()) ? &it->second : nullptr;
		}

		/// Get the current tracked state of a resource (const).
		[[nodiscard]]
		const ResourceStateEntry *GetState(const ResourceId &id) const
		{
			auto it = m_states.find(id);
			return (it != m_states.end()) ? &it->second : nullptr;
		}

		/// Update the tracked state of a resource after a transition.
		void SetState(const ResourceId &id, Hush::Graphics::EResourceState newState, uint32_t owningQueueIndex)
		{
			auto it = m_states.find(id);
			if (it != m_states.end())
			{
				it->second.currentState = newState;
				it->second.owningQueueIndex = owningQueueIndex;
			}
		}

		/// Mark a resource as being in a multi-queue read state.
		void SetMultiQueueRead(const ResourceId &id, bool value)
		{
			auto it = m_states.find(id);
			if (it != m_states.end())
			{
				it->second.inMultiQueueRead = value;
			}
		}

		/// Clear all tracked state (call between frames or on graph reset).
		void Clear()
		{
			m_states.clear();
		}

	private:
		boost::unordered_flat_map<ResourceId, ResourceStateEntry> m_states;
	};

	/// Represents a group of command lists to be submitted together in a single
	/// ExecuteCommandLists call, surrounded by fence waits and/or signals.
	///
	/// This is the fundamental execution unit described in the article's
	/// "Command list batching" section. Batches are split whenever a fence
	/// signal or wait requirement is detected while iterating through passes
	/// on a queue within a dependency level.
	struct CommandListBatch
	{
		/// Fence wait operations to perform BEFORE executing any command lists.
		/// The queue will not begin executing command lists in this batch until
		/// all wait conditions are satisfied.
		std::vector<Hush::Graphics::FenceWaitDescriptor> fenceWaits;

		/// Command lists to execute in this batch, in submission order.
		/// Each command list should already be closed (recorded and finalized).
		/// During batch building these are nullptr placeholders; during submission
		/// they are replaced with real command list pointers.
		std::vector<Hush::Graphics::ICommandList *> commandLists;

		/// Fence signal operations to perform AFTER all command lists complete.
		/// Typically signals the queue's own timeline fence to allow dependent
		/// queues to proceed.
		std::vector<Hush::Graphics::FenceSignalDescriptor> fenceSignals;

		/// Whether this batch contains the rerouted transitions command list.
		/// When true, this batch is the dedicated "transitions" batch that runs
		/// on the most competent queue before the main work batches.
		bool isTransitionRerouteBatch = false;
	};

	/// Describes the full execution plan for a single queue across one
	/// dependency level.
	///
	/// For queues involved in cross-queue dependencies, the plan may contain:
	///   1. A rerouted transitions batch (barriers from other queues).
	///   2. One or more work batches, split at fence signal/wait boundaries.
	///
	/// For queues NOT involved in cross-queue dependencies, the plan simply
	/// contains work batches with no rerouting, where split barriers can be
	/// used for better performance.
	struct QueueExecutionPlan
	{
		/// Index of the queue this plan belongs to (maps to EPassType).
		uint32_t queueIndex = 0;

		/// Ordered list of command list batches to submit on this queue.
		/// Batches are submitted sequentially; within each batch, command lists
		/// are submitted together in a single API call.
		std::vector<CommandListBatch> batches;

		/// True if this queue is involved in cross-queue transition rerouting
		/// for the current dependency level.
		bool requiresTransitionRerouting = false;

		/// True if this queue is the "most competent queue" responsible for
		/// performing rerouted resource transitions.
		bool isMostCompetentQueue = false;
	};

	/// Holds transient execution data built during Execute() for a single
	/// dependency level. This includes barriers, rerouted transitions, and
	/// the per-queue execution plans.
	struct DependencyLevelExecutionContext
	{
		/// Per-queue execution plans for this dependency level.
		/// Index corresponds to queue index.
		std::vector<QueueExecutionPlan> queuePlans;

		/// Resources that require rerouted transitions in this dependency level
		/// (read by multiple queues, or transition unsupported on receiving queue).
		std::vector<Hush::Graphics::ResourceBarrierDescriptor> reroutedBarriers;

		/// Index of the queue responsible for executing rerouted transitions.
		uint32_t mostCompetentQueueIndex = 0;
	};

	/// Executes a compiled RenderGraph on the GPU.
	///
	/// The executor is the runtime counterpart to the RenderGraph. While the
	/// graph describes what work to do and in what order (computed at compile
	/// time), the executor determines how to do it on the GPU:
	///
	///   - **Resource realization**: creates actual GPU resources from the
	///     deferred handles declared during the graph's build phase.
	///   - **Resource state tracking**: maintains the current GPU state of
	///     every resource and computes resource barriers.
	///   - **Transition rerouting**: detects when a receiving queue cannot
	///     perform a state transition and reroutes it to the "most competent
	///     queue" (usually graphics).
	///   - **Command list batching**: groups command lists between fence
	///     waits and signals for optimal submission.
	///   - **Fence management**: creates and manages per-queue timeline fences
	///     for cross-queue synchronization.
	///
	/// Owned by RenderDevice alongside the RenderGraph.
	class RenderGraphExecutor
	{
	public:
		/// Construct an executor bound to a graphics device.
		///
		/// @param device The graphics device used to create command lists, fences,
		///               and query queue capabilities. Must outlive the executor.
		explicit RenderGraphExecutor(Hush::Graphics::IGraphicsDevice *device);

		~RenderGraphExecutor() = default;

		RenderGraphExecutor(const RenderGraphExecutor &) = delete;
		RenderGraphExecutor &operator=(const RenderGraphExecutor &) = delete;
		RenderGraphExecutor(RenderGraphExecutor &&) = default;
		RenderGraphExecutor &operator=(RenderGraphExecutor &&) = default;

		/// Execute a compiled render graph.
		///
		/// @param graph A compiled (non-dirty) render graph. The graph is passed
		///              by mutable reference because execution may update
		///              per-node fence signal values.
		void Execute(RenderGraph &graph);

		/// Reset execution state between frames (or when the graph is reset).
		///
		/// Clears resource state tracking and resets fence value counters.
		/// Fences themselves are NOT destroyed (they are reused across frames).
		void ResetFrameState();

	private:
		/// Create actual GPU resources for all unrealized transient handles.
		///
		/// Called at the start of Execute(). Imported resources are skipped
		/// because they are already realized at import time.
		void RealizeResources(RenderGraph &graph);

		/// Ensure per-queue timeline fences exist for the given queue count.
		/// Creates new fences only if the vector is undersized.
		void EnsureFencesCreated(uint32_t queueCount);

		/// Allocate the next monotonically increasing fence value for a queue.
		///
		/// @param queueIndex The queue index (0=Graphics, 1=Compute, 2=Transfer).
		/// @return The next fence value to use for signaling.
		uint64_t AllocateFenceValue(uint32_t queueIndex);

		/// Compute resource barriers needed for a pass, given its read/write
		/// resource sets and the current tracked resource states.
		///
		/// @param passNode The pass to compute barriers for.
		/// @param resourceManager The graph's resource manager (for native ptrs).
		/// @param[out] barriers Output vector to receive the barriers.
		void ComputeBarriersForPass(RenderPassNode &passNode, const ResourceManager &resourceManager,
									std::vector<Hush::Graphics::ResourceBarrierDescriptor> &barriers);

		/// Determine the most competent queue for performing a resource state
		/// transition that involves the given before/after states.
		///
		/// @param stateBefore The resource state before the transition.
		/// @param stateAfter  The resource state after the transition.
		/// @param involvedQueues Set of queue indices involved.
		/// @return Queue index of the most competent queue.
		[[nodiscard]]
		uint32_t FindMostCompetentQueue(Hush::Graphics::EResourceState stateBefore,
										Hush::Graphics::EResourceState stateAfter,
										const boost::unordered_flat_set<uint32_t> &involvedQueues) const;

		/// Check if a queue supports transitioning between the given states.
		[[nodiscard]]
		bool QueueSupportsTransition(uint32_t queueIndex, Hush::Graphics::EResourceState stateBefore,
									 Hush::Graphics::EResourceState stateAfter) const;

		/// Build the execution context for a single dependency level.
		///
		/// Performs multi-queue read detection, most-competent-queue selection,
		/// rerouted barrier collection, and command list batch construction.
		DependencyLevelExecutionContext BuildExecutionContext(RenderGraph::DependencyLevel &level,
															  const ResourceManager &resourceManager,
															  uint32_t queueCount);

		/// Build command list batches for a queue involved in cross-queue
		/// transition rerouting.
		///
		/// Batches are split only at signal boundaries. The initial batch may
		/// contain a wait on the most-competent queue's transition fence.
		///
		/// @param plan           The queue's execution plan to populate with batches.
		/// @param nodesOnQueue   Passes assigned to this queue in the current level.
		/// @param execCtx        The dependency level execution context (for rerouted barrier info).
		/// @param queueIndex     The queue index being processed.
		void BuildBatchesForReroutedQueue(QueueExecutionPlan &plan, const std::vector<RenderPassNode *> &nodesOnQueue,
										  const DependencyLevelExecutionContext &execCtx, uint32_t queueIndex);

		/// Build command list batches for a queue NOT involved in cross-queue
		/// transition rerouting.
		///
		/// Batches are split at both wait and signal boundaries, allowing split
		/// barriers and better pass reordering on independent queues.
		///
		/// @param plan           The queue's execution plan to populate with batches.
		/// @param nodesOnQueue   Passes assigned to this queue in the current level.
		/// @param queueIndex     The queue index being processed.
		void BuildBatchesForIndependentQueue(QueueExecutionPlan &plan,
											 const std::vector<RenderPassNode *> &nodesOnQueue, uint32_t queueIndex);

		/// Get the ICommandQueue* for a given queue index.
		[[nodiscard]]
		Hush::Graphics::ICommandQueue *GetQueueByIndex(uint32_t queueIndex) const;

		/// Map queue index to EQueueType.
		[[nodiscard]]
		static Hush::Graphics::EQueueType QueueIndexToType(uint32_t queueIndex)
		{
			switch (queueIndex)
			{
			case 1:
				return Hush::Graphics::EQueueType::Compute;
			case 2:
				return Hush::Graphics::EQueueType::Transfer;
			default:
				return Hush::Graphics::EQueueType::Graphics;
			}
		}

		/// Create a new command list appropriate for the given queue index.
		[[nodiscard]]
		std::unique_ptr<Hush::Graphics::ICommandList> CreateCommandListForQueue(uint32_t queueIndex) const;

	private:
		/// Non-owning pointer to the graphics device. Must outlive the executor.
		Hush::Graphics::IGraphicsDevice *m_device = nullptr;

		/// Per-resource GPU state, initialized at the start of each Execute()
		/// call and updated as barriers are inserted.
		ResourceStateTracker m_stateTracker;

		/// Per-queue timeline fences. Index corresponds to queue index.
		/// Created lazily and reused across frames.
		std::vector<std::unique_ptr<Hush::Graphics::IFence>> m_queueFences;

		/// Per-queue monotonically increasing fence value counters.
		/// Reset each frame via ResetFrameState().
		std::vector<uint64_t> m_queueFenceValues;
	};

} // namespace Hush::RenderGraph
