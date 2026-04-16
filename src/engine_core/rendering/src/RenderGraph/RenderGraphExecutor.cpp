/*! \file RenderGraphExecutor.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-18
	\brief RenderGraph executor implementation — all GPU execution infrastructure.

	This file implements the runtime execution of a compiled RenderGraph:
	  - Resource realization (creating GPU resources from deferred handles)
	  - Resource state tracking and barrier computation
	  - Transition rerouting to the "most competent queue"
	  - Command list batching with fence waits/signals
	  - Per-pass command list recording and batched submission
*/

#include "RenderGraphExecutor.hpp"
#include <Assertions.hpp>
#include <Profiling.hpp>
#include <algorithm>

using namespace Hush::RenderGraph;

RenderGraphExecutor::RenderGraphExecutor(Hush::Graphics::IGraphicsDevice *device)
	: m_device(device)
{
	HUSH_ASSERT(m_device != nullptr, "RenderGraphExecutor requires a valid graphics device!");
}

void RenderGraphExecutor::ResetFrameState()
{
	m_stateTracker.Clear();

	// Reset fence value counters but keep the fence objects alive for reuse
	std::fill(m_queueFenceValues.begin(), m_queueFenceValues.end(), 0);
}

void RenderGraphExecutor::RealizeResources(RenderGraph &graph)
{
	ZoneScoped;
	graph.GetResourceManager().ForEachResource([&](ResourceId /*id*/, ResourceHandle &handle) {
		if (!handle.IsRealized())
		{
			handle.CreateResource(m_device);
		}
	});
}

void RenderGraphExecutor::EnsureFencesCreated(uint32_t queueCount)
{
	if (m_queueFences.size() >= queueCount)
	{
		return;
	}

	const size_t oldSize = m_queueFences.size();
	m_queueFences.resize(queueCount);
	m_queueFenceValues.resize(queueCount, 0);

	for (size_t q = oldSize; q < queueCount; ++q)
	{
		m_queueFences[q] = m_device->CreateFence(0);
		HUSH_ASSERT(m_queueFences[q] != nullptr, "Failed to create timeline fence for queue");
	}
}

uint64_t RenderGraphExecutor::AllocateFenceValue(uint32_t queueIndex)
{
	HUSH_ASSERT(queueIndex < m_queueFenceValues.size(), "Queue index out of range");
	return ++m_queueFenceValues[queueIndex];
}

Hush::Graphics::ICommandQueue *RenderGraphExecutor::GetQueueByIndex(uint32_t queueIndex) const
{
	return m_device->GetQueueForType(QueueIndexToType(queueIndex));
}

std::unique_ptr<Hush::Graphics::ICommandList> RenderGraphExecutor::CreateCommandListForQueue(uint32_t queueIndex) const
{
	switch (queueIndex)
	{
	case 1:
		return m_device->CreateComputeCommandList();
	case 2:
		return m_device->CreateCopyCommandList();
	default:
		return m_device->CreateGraphicsCommandList();
	}
}

bool RenderGraphExecutor::QueueSupportsTransition(uint32_t queueIndex, Hush::Graphics::EResourceState stateBefore,
												  Hush::Graphics::EResourceState stateAfter) const
{
	const uint32_t supportedStates = m_device->GetQueueSupportedStates(QueueIndexToType(queueIndex));
	const auto beforeBits = static_cast<uint32_t>(stateBefore);
	const auto afterBits = static_cast<uint32_t>(stateAfter);

	// Queue must support both the before and after states to perform the transition
	return (beforeBits & ~supportedStates) == 0 && (afterBits & ~supportedStates) == 0;
}

uint32_t RenderGraphExecutor::FindMostCompetentQueue(Hush::Graphics::EResourceState stateBefore,
													 Hush::Graphics::EResourceState stateAfter,
													 const boost::unordered_flat_set<uint32_t> &involvedQueues) const
{
	// Prefer involved queues first, then fall back to graphics (queue 0) which supports all states
	for (uint32_t q : involvedQueues)
	{
		if (QueueSupportsTransition(q, stateBefore, stateAfter))
		{
			return q;
		}
	}

	// Graphics queue is always the most competent fallback
	return 0;
}

void RenderGraphExecutor::ComputeBarriersForPass(RenderPassNode &passNode, const ResourceManager &resourceManager,
												 std::vector<Hush::Graphics::ResourceBarrierDescriptor> &barriers)
{
	// Compute barriers for read resources
	for (const ResourceId &rid : passNode.GetReadResources())
	{
		ResourceStateEntry *stateEntry = m_stateTracker.GetState(rid);
		if (stateEntry == nullptr)
		{
			continue;
		}

		Hush::Graphics::EResourceState desiredState = passNode.GetReadState(rid);
		if (desiredState == Hush::Graphics::EResourceState::Undefined)
		{
			continue; // No explicit state requested
		}

		if (stateEntry->currentState != desiredState)
		{
			Hush::RenderGraph::ResourceHandle *handle = resourceManager.GetResourceHandle(rid);
			void *resource = (handle != nullptr) ? handle->GetNativePtr() : nullptr;

			barriers.push_back(Hush::Graphics::ResourceBarrierDescriptor{.resource = resource,
																		 .stateBefore = stateEntry->currentState,
																		 .stateAfter = desiredState,
																		 .subresource = UINT32_MAX});

			// Update tracked state
			m_stateTracker.SetState(rid, desiredState, passNode.GetQueueIndex());
		}
	}

	// Compute barriers for written resources
	for (const ResourceId &rid : passNode.GetWrittenResources())
	{
		ResourceStateEntry *stateEntry = m_stateTracker.GetState(rid);
		if (stateEntry == nullptr)
		{
			continue;
		}

		Hush::Graphics::EResourceState desiredState = passNode.GetWriteState(rid);
		if (desiredState == Hush::Graphics::EResourceState::Undefined)
		{
			continue;
		}

		if (stateEntry->currentState != desiredState)
		{
			Hush::RenderGraph::ResourceHandle *handle = resourceManager.GetResourceHandle(rid);
			void *resource = (handle != nullptr) ? handle->GetNativePtr() : nullptr;

			barriers.push_back(Hush::Graphics::ResourceBarrierDescriptor{.resource = resource,
																		 .stateBefore = stateEntry->currentState,
																		 .stateAfter = desiredState,
																		 .subresource = UINT32_MAX});

			m_stateTracker.SetState(rid, desiredState, passNode.GetQueueIndex());
		}
	}
}

DependencyLevelExecutionContext RenderGraphExecutor::BuildExecutionContext(RenderGraph::DependencyLevel &level,
																		   const ResourceManager &resourceManager,
																		   uint32_t queueCount)
{
	ZoneScoped;
	DependencyLevelExecutionContext execCtx;
	execCtx.queuePlans.resize(queueCount);

	for (uint32_t q = 0; q < queueCount; ++q)
	{
		execCtx.queuePlans[q].queueIndex = q;
	}

	// --- Step 1: Read cross-queue dependency info (already populated during compilation) ---
	const auto &crossDepQueues = level.GetQueuesInvolvedInCrossDependencies();
	const auto &multiQueueResources = level.GetResourcesReadByMultipleQueues();

	const bool hasCrossQueueDeps = !crossDepQueues.empty();

	// --- Step 2: Determine most competent queue for transition rerouting ---
	if (hasCrossQueueDeps)
	{
		// Collect all cross-queue transitions to find the combined before/after states
		Hush::Graphics::EResourceState combinedBefore = Hush::Graphics::EResourceState::Undefined;
		Hush::Graphics::EResourceState combinedAfter = Hush::Graphics::EResourceState::Undefined;

		for (const ResourceId &rid : multiQueueResources)
		{
			const ResourceStateEntry *stateEntry = m_stateTracker.GetState(rid);
			if (stateEntry != nullptr)
			{
				combinedBefore = combinedBefore | stateEntry->currentState;
			}

			// Collect the combined desired read state from all passes reading this resource
			for (RenderPassNode *node : level.GetPassNodes())
			{
				Hush::Graphics::EResourceState readState = node->GetReadState(rid);
				if (readState != Hush::Graphics::EResourceState::Undefined)
				{
					combinedAfter = combinedAfter | readState;
				}
			}
		}

		execCtx.mostCompetentQueueIndex = FindMostCompetentQueue(combinedBefore, combinedAfter, crossDepQueues);

		// Mark queues involved in cross-dependencies as requiring transition rerouting
		for (uint32_t q : crossDepQueues)
		{
			execCtx.queuePlans[q].requiresTransitionRerouting = true;
		}
		execCtx.queuePlans[execCtx.mostCompetentQueueIndex].isMostCompetentQueue = true;

		// --- Step 3: Build rerouted barriers for multi-queue reads ---
		for (const ResourceId &rid : multiQueueResources)
		{
			ResourceStateEntry *stateEntry = m_stateTracker.GetState(rid);
			if (stateEntry == nullptr)
			{
				continue;
			}

			// Compute combined read state across all queues for this resource
			Hush::Graphics::EResourceState combinedReadState = Hush::Graphics::EResourceState::Undefined;
			for (RenderPassNode *node : level.GetPassNodes())
			{
				Hush::Graphics::EResourceState readState = node->GetReadState(rid);
				if (readState != Hush::Graphics::EResourceState::Undefined)
				{
					combinedReadState = combinedReadState | readState;
				}
			}

			if (stateEntry->currentState != combinedReadState &&
				combinedReadState != Hush::Graphics::EResourceState::Undefined)
			{
				Hush::RenderGraph::ResourceHandle *handle = resourceManager.GetResourceHandle(rid);
				void *resource = (handle != nullptr) ? handle->GetNativePtr() : nullptr;

				execCtx.reroutedBarriers.push_back(
					Hush::Graphics::ResourceBarrierDescriptor{.resource = resource,
															  .stateBefore = stateEntry->currentState,
															  .stateAfter = combinedReadState,
															  .subresource = UINT32_MAX});

				// Update tracked state: after rerouted transition, resource is in combined read state
				m_stateTracker.SetState(rid, combinedReadState, execCtx.mostCompetentQueueIndex);
				m_stateTracker.SetMultiQueueRead(rid, true);
			}
		}
	}

	// --- Step 4: Build batches per queue ---
	const auto &nodesPerQueue = level.GetNodesPerQueue();

	for (uint32_t q = 0; q < queueCount; ++q)
	{
		QueueExecutionPlan &plan = execCtx.queuePlans[q];

		const bool hasNodesOnQueue = (nodesPerQueue.size() > q) && !nodesPerQueue[q].empty();

		if (!hasNodesOnQueue)
		{
			// No work on this queue, but if it's the most competent queue and there
			// are rerouted barriers, we still need a transitions batch.
			if (plan.isMostCompetentQueue && !execCtx.reroutedBarriers.empty())
			{
				CommandListBatch transitionBatch;
				transitionBatch.isTransitionRerouteBatch = true;

				// Wait on all other queues involved in cross-dependencies
				for (uint32_t otherQ : crossDepQueues)
				{
					if (otherQ != q && m_queueFenceValues[otherQ] > 0)
					{
						transitionBatch.fenceWaits.push_back(Hush::Graphics::FenceWaitDescriptor{
							.fence = m_queueFences[otherQ].get(), .value = m_queueFenceValues[otherQ]});
					}
				}

				uint64_t signalVal = AllocateFenceValue(q);
				transitionBatch.fenceSignals.push_back(
					Hush::Graphics::FenceSignalDescriptor{.fence = m_queueFences[q].get(), .value = signalVal});

				plan.batches.push_back(std::move(transitionBatch));
			}
			continue;
		}

		const auto &nodesOnQueue = nodesPerQueue[q];

		// If this queue IS the most competent queue, insert the rerouted
		// transitions batch first.
		if (plan.isMostCompetentQueue && !execCtx.reroutedBarriers.empty())
		{
			CommandListBatch transitionBatch;
			transitionBatch.isTransitionRerouteBatch = true;

			// Wait on all other queues involved in cross-deps that have submitted work
			for (uint32_t otherQ : crossDepQueues)
			{
				if (otherQ != q && m_queueFenceValues[otherQ] > 0)
				{
					transitionBatch.fenceWaits.push_back(Hush::Graphics::FenceWaitDescriptor{
						.fence = m_queueFences[otherQ].get(), .value = m_queueFenceValues[otherQ]});
				}
			}

			uint64_t transitionSignalVal = AllocateFenceValue(q);
			transitionBatch.fenceSignals.push_back(
				Hush::Graphics::FenceSignalDescriptor{.fence = m_queueFences[q].get(), .value = transitionSignalVal});

			plan.batches.push_back(std::move(transitionBatch));
		}

		// Build work batches for this queue.
		// Batches are split when a fence signal or wait is required.
		if (plan.requiresTransitionRerouting)
		{
			BuildBatchesForReroutedQueue(plan, nodesOnQueue, execCtx, q);
		}
		else
		{
			BuildBatchesForIndependentQueue(plan, nodesOnQueue, q);
		}
	}

	return execCtx;
}

void RenderGraphExecutor::BuildBatchesForReroutedQueue(QueueExecutionPlan &plan,
													   const std::vector<RenderPassNode *> &nodesOnQueue,
													   const DependencyLevelExecutionContext &execCtx,
													   uint32_t queueIndex)
{
	// Queues involved in cross-deps: batch all passes together, separated
	// only by signal requirements. The initial batch may have a wait on
	// the most-competent queue's transition fence.
	CommandListBatch currentBatch;

	// If not the most competent queue, wait on the transition fence
	if (!plan.isMostCompetentQueue && !execCtx.reroutedBarriers.empty())
	{
		uint32_t mcq = execCtx.mostCompetentQueueIndex;
		if (m_queueFenceValues[mcq] > 0)
		{
			currentBatch.fenceWaits.push_back(Hush::Graphics::FenceWaitDescriptor{.fence = m_queueFences[mcq].get(),
																				  .value = m_queueFenceValues[mcq]});
		}
	}
	// If IS the most competent queue, wait on own fence (transition batch signaled it)
	else if (plan.isMostCompetentQueue && !execCtx.reroutedBarriers.empty())
	{
		currentBatch.fenceWaits.push_back(Hush::Graphics::FenceWaitDescriptor{.fence = m_queueFences[queueIndex].get(),
																			  .value = m_queueFenceValues[queueIndex]});
	}

	for (RenderPassNode *passNode : nodesOnQueue)
	{
		// If this pass must signal (it has cross-queue dependents), we need
		// to close the current batch with a signal and start a new one.
		if (passNode->IsSyncSignalRequired() && !currentBatch.commandLists.empty())
		{
			uint64_t sigVal = AllocateFenceValue(queueIndex);
			passNode->m_fenceSignalValue = sigVal;
			currentBatch.fenceSignals.push_back(
				Hush::Graphics::FenceSignalDescriptor{.fence = m_queueFences[queueIndex].get(), .value = sigVal});

			plan.batches.push_back(std::move(currentBatch));
			currentBatch = CommandListBatch{};
		}

		// nullptr placeholder — filled during submission with a real command list
		currentBatch.commandLists.push_back(nullptr);

		// If this pass must signal and it's the only (or last) entry
		if (passNode->IsSyncSignalRequired() && currentBatch.fenceSignals.empty())
		{
			uint64_t sigVal = AllocateFenceValue(queueIndex);
			passNode->m_fenceSignalValue = sigVal;
			currentBatch.fenceSignals.push_back(
				Hush::Graphics::FenceSignalDescriptor{.fence = m_queueFences[queueIndex].get(), .value = sigVal});
		}
	}

	// Close final batch with a signal so downstream levels can depend on it
	if (!currentBatch.commandLists.empty())
	{
		if (currentBatch.fenceSignals.empty())
		{
			uint64_t sigVal = AllocateFenceValue(queueIndex);
			currentBatch.fenceSignals.push_back(
				Hush::Graphics::FenceSignalDescriptor{.fence = m_queueFences[queueIndex].get(), .value = sigVal});
		}
		plan.batches.push_back(std::move(currentBatch));
	}
}

void RenderGraphExecutor::BuildBatchesForIndependentQueue(QueueExecutionPlan &plan,
														  const std::vector<RenderPassNode *> &nodesOnQueue,
														  uint32_t queueIndex)
{
	// Queues NOT involved in cross-deps: batch passes, split on wait/signal.
	CommandListBatch currentBatch;

	for (RenderPassNode *passNode : nodesOnQueue)
	{
		// Check if this pass needs to wait on other queues
		bool needsWait = false;
		for (RenderPassNode *syncNode : passNode->GetNodesToSync())
		{
			if (syncNode->GetQueueIndex() != queueIndex && syncNode->m_fenceSignalValue > 0)
			{
				needsWait = true;
				break;
			}
		}

		// If needs wait, start a new batch (the waiting pass goes into the new batch)
		if (needsWait && !currentBatch.commandLists.empty())
		{
			plan.batches.push_back(std::move(currentBatch));
			currentBatch = CommandListBatch{};
		}

		// Add waits for this pass
		if (needsWait)
		{
			for (RenderPassNode *syncNode : passNode->GetNodesToSync())
			{
				if (syncNode->GetQueueIndex() != queueIndex && syncNode->m_fenceSignalValue > 0)
				{
					currentBatch.fenceWaits.push_back(
						Hush::Graphics::FenceWaitDescriptor{.fence = m_queueFences[syncNode->GetQueueIndex()].get(),
															.value = syncNode->m_fenceSignalValue});
				}
			}
		}

		// nullptr placeholder — filled during submission
		currentBatch.commandLists.push_back(nullptr);

		// If this pass must signal, close this batch
		if (passNode->IsSyncSignalRequired())
		{
			uint64_t sigVal = AllocateFenceValue(queueIndex);
			passNode->m_fenceSignalValue = sigVal;
			currentBatch.fenceSignals.push_back(
				Hush::Graphics::FenceSignalDescriptor{.fence = m_queueFences[queueIndex].get(), .value = sigVal});

			plan.batches.push_back(std::move(currentBatch));
			currentBatch = CommandListBatch{};
		}
	}

	// Close final batch
	if (!currentBatch.commandLists.empty())
	{
		if (currentBatch.fenceSignals.empty())
		{
			uint64_t sigVal = AllocateFenceValue(queueIndex);
			currentBatch.fenceSignals.push_back(
				Hush::Graphics::FenceSignalDescriptor{.fence = m_queueFences[queueIndex].get(), .value = sigVal});
		}
		plan.batches.push_back(std::move(currentBatch));
	}
}

void RenderGraphExecutor::Execute(RenderGraph &graph)
{
	ZoneScoped;
	HUSH_ASSERT(graph.IsCompiled(), "Cannot execute a dirty render graph! Call Compile() first.");
	HUSH_ASSERT(m_device != nullptr, "Graphics device cannot be null!");

	const uint32_t queueCount = graph.GetDetectedQueueCount();

	// --- Step 1: Realize any unrealized transient resources ---
	RealizeResources(graph);

	// --- Step 2: Initialize resource state tracking from the graph ---
	m_stateTracker.InitializeFromGraph(graph);

	// --- Step 3: Ensure per-queue timeline fences exist ---
	EnsureFencesCreated(queueCount);

	// Keep all command lists alive for the duration of execution.
	// Command lists are created per-pass and must survive until their
	// corresponding queue submission completes.
	std::vector<std::unique_ptr<Hush::Graphics::ICommandList>> ownedCommandLists;

	ResourceManager &resourceManager = graph.GetResourceManager();

	// --- Step 4: Execute passes level by level ---
	for (RenderGraph::DependencyLevel &level : graph.GetDependencyLevels())
	{
		// Build the execution context: barriers, rerouting, batches
		DependencyLevelExecutionContext execCtx = BuildExecutionContext(level, resourceManager, queueCount);

		const auto &nodesPerQueue = level.GetNodesPerQueue();

		// Record and submit each queue's batches
		for (uint32_t q = 0; q < queueCount; ++q)
		{
			QueueExecutionPlan &plan = execCtx.queuePlans[q];
			if (plan.batches.empty())
			{
				continue;
			}

			Hush::Graphics::ICommandQueue *queue = GetQueueByIndex(q);

			// Per-queue pass iterator: consumed as we fill batches with real command lists
			const bool hasNodesOnQueue = (nodesPerQueue.size() > q) && !nodesPerQueue[q].empty();
			const auto &queueNodes = hasNodesOnQueue ? nodesPerQueue[q] : std::vector<RenderPassNode *>{};
			size_t passIndex = 0;

			for (CommandListBatch &batch : plan.batches)
			{
				Hush::Graphics::SubmitInfo submitInfo;
				submitInfo.waitFences = batch.fenceWaits;
				submitInfo.signalFences = batch.fenceSignals;

				if (batch.isTransitionRerouteBatch)
				{
					// --- Rerouted transitions batch ---
					// Create a dedicated command list, record all rerouted barriers,
					// and submit it with the batch's fence waits/signals.
					auto transitionCmd = CreateCommandListForQueue(q);
					transitionCmd->Reset();

					if (!execCtx.reroutedBarriers.empty())
					{
						transitionCmd->ResourceBarrier(std::span<const Hush::Graphics::ResourceBarrierDescriptor>(
							execCtx.reroutedBarriers.data(), execCtx.reroutedBarriers.size()));
					}

					transitionCmd->Close();
					submitInfo.commandLists.push_back(transitionCmd.get());
					ownedCommandLists.push_back(std::move(transitionCmd));
				}
				else
				{
					// --- Work batch ---
					// Each nullptr in batch.commandLists corresponds to one pass
					// from the per-queue node list, consumed in order.
					for (size_t i = 0; i < batch.commandLists.size(); ++i)
					{
						if (passIndex >= queueNodes.size())
						{
							break;
						}

						RenderPassNode *passNode = queueNodes[passIndex];
						++passIndex;

						// Create a command list for this pass
						auto cmdList = CreateCommandListForQueue(q);
						cmdList->Reset();

						// Compute and record resource barriers for this pass
						std::vector<Hush::Graphics::ResourceBarrierDescriptor> passBarriers;
						ComputeBarriersForPass(*passNode, resourceManager, passBarriers);

						if (!passBarriers.empty())
						{
							cmdList->ResourceBarrier(std::span<const Hush::Graphics::ResourceBarrierDescriptor>(
								passBarriers.data(), passBarriers.size()));
						}

						// Execute the pass's recorded work
						passNode->Execute(cmdList.get(), resourceManager);

						cmdList->Close();
						submitInfo.commandLists.push_back(cmdList.get());
						ownedCommandLists.push_back(std::move(cmdList));
					}
				}

				// Submit the batch with its fence waits and signals
				queue->SubmitBatched(submitInfo);
			}
		}
	}
}
