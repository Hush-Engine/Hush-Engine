/*! \file RenderGraph.cpp
	\author Alan Ramirez Herrera
	\date 2025-11-17
	\brief RenderGraph implementation for rendering based on DAG scheduling
*/

#include "RenderGraph.hpp"
#include "RHI/ICommandList.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include <Assertions.hpp>
#include <algorithm>

using namespace Hush::RenderGraph;

void RenderGraph::Compile()
{
	if (m_state == ERenderGraphState::Compiled)
	{
		return; // Already compiled
	}

	// Clear previous compilation data
	m_adjacencyList.clear();
	m_topologicalOrderedNodes.clear();
	m_dependencyLevels.clear();
	m_detectedQueueCount = 1;

	// Build and optimize the graph
	BuildAdjacencyList();
	TopologicalSort();
	BuildDependencyLevels();
	FinalizeDependencyLevels();
	CullRedundantSyncPoints();

	m_state = ERenderGraphState::Compiled;
}

void RenderGraph::BuildAdjacencyList()
{
	m_adjacencyList.resize(m_passes.size());

	// Build adjacency list by checking write-to-read dependencies
	for (uint32_t nodeIndex = 0; nodeIndex < m_passes.size(); ++nodeIndex)
	{
		RenderPassNode &passNode = m_passes[nodeIndex];
		std::vector<uint32_t> &adjacentNodeIndices = m_adjacencyList[nodeIndex];

		for (uint32_t otherNodeIndex = 0; otherNodeIndex < m_passes.size(); ++otherNodeIndex)
		{
			if (nodeIndex == otherNodeIndex)
			{
				continue; // Skip self-dependency
			}

			RenderPassNode &otherPassNode = m_passes[otherNodeIndex];

			// Check if otherPassNode reads any resource written by passNode
			for (const ResourceId &otherReadResource : otherPassNode.m_readResources)
			{
				if (passNode.WritesResource(otherReadResource))
				{
					// passNode -> otherPassNode dependency
					adjacentNodeIndices.push_back(otherNodeIndex);

					// Check for cross-queue dependency
					if (otherPassNode.m_queueIndex != passNode.m_queueIndex)
					{
						passNode.SetHasCrossDependency(otherPassNode);
					}

					break; // No need to check other resources
				}
			}
		}
	}
}

void RenderGraph::DFS(uint32_t nodeIndex, std::vector<bool> &visited, std::vector<bool> &onStack, bool &isCyclic)
{
	if (isCyclic)
	{
		return;
	}

	visited[nodeIndex] = true;
	onStack[nodeIndex] = true;

	// Visit all adjacent nodes
	for (uint32_t neighborIndex : m_adjacencyList[nodeIndex])
	{
		if (onStack[neighborIndex])
		{
			// We found a back edge, indicating a cycle
			isCyclic = true;
			return;
		}

		if (!visited[neighborIndex])
		{
			DFS(neighborIndex, visited, onStack, isCyclic);
		}
	}

	onStack[nodeIndex] = false;

	// Add to result in post-order
	m_topologicalOrderedNodes.push_back(&m_passes[nodeIndex]);
}

void RenderGraph::TopologicalSort()
{
	m_topologicalOrderedNodes.clear();
	std::vector<bool> visited(m_passes.size(), false);
	std::vector<bool> onStack(m_passes.size(), false);
	bool isCyclic = false;

	for (uint32_t nodeIndex = 0; nodeIndex < m_passes.size(); ++nodeIndex)
	{
		if (!visited[nodeIndex])
		{
			DFS(nodeIndex, visited, onStack, isCyclic);

			HUSH_ASSERT(!isCyclic, "Render graph contains cycles! Cannot proceed with execution.");
		}
	}

	// Reverse to get correct topological order
	std::reverse(m_topologicalOrderedNodes.begin(), m_topologicalOrderedNodes.end());
}

void RenderGraph::BuildDependencyLevels()
{
	// Longest path algorithm to determine dependency levels
	std::vector<int32_t> longestPathLengths(m_passes.size(), 0);

	uint32_t maxLevelIndex = 0;

	// Calculate longest path for each node
	for (RenderPassNode *node : m_topologicalOrderedNodes)
	{
		const uint32_t nodeOriginalIndex = node->m_unorderedPassIndex;

		// Update distances to adjacent nodes
		for (uint32_t adjacentNodeIndex : m_adjacencyList[nodeOriginalIndex])
		{
			int32_t newPathLength = longestPathLengths[nodeOriginalIndex] + 1;
			if (longestPathLengths[adjacentNodeIndex] < newPathLength)
			{
				longestPathLengths[adjacentNodeIndex] = newPathLength;
				maxLevelIndex = std::max(maxLevelIndex, static_cast<uint32_t>(newPathLength));
			}
		}
	}

	// Resize to accommodate all levels (0-indexed)
	m_dependencyLevels.resize(maxLevelIndex + 1);

	// Assign nodes to their dependency levels
	uint32_t i = 0;
	for (RenderPassNode &node : m_passes)
	{
		const auto levelIndex = static_cast<uint32_t>(longestPathLengths[i]);

		DependencyLevel &level = m_dependencyLevels[levelIndex];
		level.m_levelIndex = levelIndex;
		level.AddNode(&node);

		node.m_dependencyLevelIndex = levelIndex;

		// Track maximum queue index
		m_detectedQueueCount = std::max(m_detectedQueueCount, node.m_queueIndex + 1);
		++i;
	}
}

void RenderGraph::FinalizeDependencyLevels()
{
	// Organize passes per queue within each dependency level
	for (DependencyLevel &level : m_dependencyLevels)
	{
		level.m_nodesPerQueue.resize(m_detectedQueueCount);

		for (RenderPassNode *node : level.m_passNodes)
		{
			const uint32_t queueIndex = node->m_queueIndex;
			level.m_nodesPerQueue[queueIndex].push_back(node);

			// Track queues involved in cross-dependencies
			if (node->m_syncSignalRequired)
			{
				level.m_queuesInvolvedInCrossDependencies.insert(queueIndex);
			}
		}
	}
}

void RenderGraph::CullRedundantSyncPoints()
{
	// Initialize SSIS (Sufficient Synchronization Index Set) for each node
	for (RenderPassNode &node : m_passes)
	{
		node.m_syncIndexSet.resize(m_detectedQueueCount, RenderPassNode::INVALID_SYNC_INDEX);
	}

	// First pass: Build SSIS by finding closest dependencies on each queue
	for (DependencyLevel &level : m_dependencyLevels)
	{
		for (RenderPassNode *passNode : level.m_passNodes)
		{
			// Initialize own queue index with node's own index
			passNode->m_syncIndexSet[passNode->m_queueIndex] = passNode->m_unorderedPassIndex;

			// Find closest dependency on each other queue
			for (RenderPassNode *depNode : passNode->m_nodesToSync)
			{
				const uint32_t depQueueIndex = depNode->m_queueIndex;
				const uint32_t depNodeIndex = depNode->m_unorderedPassIndex;

				uint64_t &currentClosest = passNode->m_syncIndexSet[depQueueIndex];

				if (currentClosest == RenderPassNode::INVALID_SYNC_INDEX || depNodeIndex > currentClosest)
				{
					currentClosest = depNodeIndex;
				}
			}
		}
	}

	// Second pass: Cull redundant synchronizations using SSIS comparison
	for (DependencyLevel &level : m_dependencyLevels)
	{
		for (RenderPassNode *passNode : level.m_passNodes)
		{
			if (passNode->m_nodesToSync.empty())
			{
				continue;
			}

			// Build list of queues we need to sync with
			std::vector<uint32_t> queuesToSyncWith;
			for (uint32_t q = 0; q < m_detectedQueueCount; ++q)
			{
				if (q != passNode->m_queueIndex && passNode->m_syncIndexSet[q] != RenderPassNode::INVALID_SYNC_INDEX)
				{
					queuesToSyncWith.push_back(q);
				}
			}

			if (queuesToSyncWith.empty())
			{
				passNode->m_nodesToSync.clear();
				continue;
			}

			// Iteratively find minimal set of nodes to sync with
			std::vector<RenderPassNode *> nodesToKeep;
			auto remainingDeps = passNode->m_nodesToSync;

			while (!queuesToSyncWith.empty() && !remainingDeps.empty())
			{
				RenderPassNode *bestNode = nullptr;
				uint32_t maxQueuesCovered = 0;

				// Find node that covers maximum queues
				for (RenderPassNode *depNode : remainingDeps)
				{
					uint32_t queuesCovered = 0;

					for (uint32_t q : queuesToSyncWith)
					{
						// Compare SSIS values
						uint64_t requiredSyncIndex = passNode->m_syncIndexSet[q];
						uint64_t providedSyncIndex = depNode->m_syncIndexSet[q];

						// For same queue, adjust by -1 due to SSIS assignment rule
						if (q == passNode->m_queueIndex && requiredSyncIndex != RenderPassNode::INVALID_SYNC_INDEX)
						{
							requiredSyncIndex--;
						}

						if (providedSyncIndex != RenderPassNode::INVALID_SYNC_INDEX &&
							requiredSyncIndex <= providedSyncIndex)
						{
							queuesCovered++;
						}
					}

					if (queuesCovered > maxQueuesCovered)
					{
						maxQueuesCovered = queuesCovered;
						bestNode = depNode;
					}
				}

				if (bestNode != nullptr && maxQueuesCovered > 0)
				{
					nodesToKeep.push_back(bestNode);

					// Remove covered queues
					auto newEnd = std::remove_if(queuesToSyncWith.begin(), queuesToSyncWith.end(), [&](uint32_t q) {
						uint64_t requiredSyncIndex = passNode->m_syncIndexSet[q];
						uint64_t providedSyncIndex = bestNode->m_syncIndexSet[q];

						if (q == passNode->m_queueIndex && requiredSyncIndex != RenderPassNode::INVALID_SYNC_INDEX)
						{
							requiredSyncIndex--;
						}

						return providedSyncIndex != RenderPassNode::INVALID_SYNC_INDEX &&
							   requiredSyncIndex <= providedSyncIndex;
					});
					queuesToSyncWith.erase(newEnd, queuesToSyncWith.end());

					// Remove best node from remaining deps
					remainingDeps.erase(std::remove(remainingDeps.begin(), remainingDeps.end(), bestNode),
										remainingDeps.end());
				}
				else
				{
					break; // No more beneficial nodes
				}
			}

			// Replace with culled list
			passNode->m_nodesToSync = nodesToKeep;
		}
	}
}

void RenderGraph::Execute()
{
    HUSH_ASSERT(!IsDirty(), "Cannot execute dirty render graph! Call Compile() first.");
    HUSH_ASSERT(m_device != nullptr, "Graphics device cannot be null!");

    auto graphicsCmd = m_device->CreateGraphicsCommandList();
	auto computeCmd = m_device->CreateComputeCommandList();
	auto transferCmd = m_device->CreateCopyCommandList();

	// Execute passes level by level
	for (const DependencyLevel &level : m_dependencyLevels)
	{
		// Execute all passes in this level
		// Passes within a level can potentially execute in parallel
		for (RenderPassNode *passNode : level.m_passNodes)
		{
			// TODO: Handle synchronization points
			// if (passNode->m_syncSignalRequired) { /* signal */ }
			// for (auto* syncNode : passNode->m_nodesToSync) { /* wait */ }

			if (passNode->m_queueIndex == 0)
			{
				passNode->Execute(graphicsCmd.get(), m_resourceManager);
			}
			else if (passNode->m_queueIndex == 1)
			{
				passNode->Execute(graphicsCmd.get(), m_resourceManager);
			}
			else if (passNode->m_queueIndex == 2)
			{
				passNode->Execute(transferCmd.get(), m_resourceManager);
			}
		}
	}

	// After executing all passes, submit command lists to the device
    if (graphicsCmd)
    {
        graphicsCmd->Close();
        std::array<Hush::Graphics::ICommandList*, 1> cmdLists = {graphicsCmd.get()};
		m_device->GetGraphicsQueue()->Submit(cmdLists);
    }

    if (computeCmd)
    {
        computeCmd->Close();
        std::array<Hush::Graphics::ICommandList*, 1> cmdLists = {computeCmd.get()};
		m_device->GetComputeQueue()->Submit(cmdLists);
    }

    if (transferCmd)
    {
        transferCmd->Close();
        std::array<Hush::Graphics::ICommandList*, 1> cmdLists = {transferCmd.get()};
		m_device->GetTransferQueue()->Submit(cmdLists);
    }
}
