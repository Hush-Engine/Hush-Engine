/*! \file RenderGraph.cpp
	\author Alan Ramirez Herrera
	\date 2025-11-17
	\brief RenderGraph compilation implementation based on DAG scheduling.

	This file contains only graph compilation logic: adjacency list construction,
	topological sorting, dependency level assignment, and SSIS-based synchronization
	point culling. All execution logic (fences, barriers, batching, submission)
	lives in RenderGraphExecutor.cpp.
*/

#include "RenderGraph.hpp"
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

		// Detect multi-queue reads for transition rerouting
		level.DetectMultiQueueReads();
	}
}

void RenderGraph::CullRedundantSyncPoints()
{
	// Initialize SSIS for each node
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

bool Hush::RenderGraph::RenderPassNode::WritesResource(ResourceId id) const
{
	return m_writtenResources.find(id) != m_writtenResources.end();
}

void Hush::RenderGraph::RenderPassNode::SetHasCrossDependency(RenderPassNode &other)
{
	m_syncSignalRequired = true;
	other.m_nodesToSync.push_back(this);
}

void Hush::RenderGraph::RenderPassNode::Execute(Hush::Graphics::ICommandList *cmdList, ResourceManager &resourceManager)
{
	m_pass->Execute(cmdList, resourceManager);
}

void Hush::RenderGraph::RenderPassNode::AddReadResource(ResourceId id)
{
	m_readResources.insert(id);
}
void Hush::RenderGraph::RenderPassNode::AddWrittenResource(ResourceId id)
{
	m_writtenResources.insert(id);
}
void Hush::RenderGraph::RenderPassNode::SetReadState(ResourceId id, Hush::Graphics::EResourceState state)
{
	m_resourceReadStates[id] = state;
}
Hush::Graphics::EResourceState Hush::RenderGraph::RenderPassNode::GetReadState(ResourceId id) const
{
	auto it = m_resourceReadStates.find(id);
	return it != m_resourceReadStates.end() ? it->second : Hush::Graphics::EResourceState::Undefined;
}
Hush::Graphics::EResourceState Hush::RenderGraph::RenderPassNode::GetWriteState(ResourceId id) const
{
	auto it = m_resourceWriteStates.find(id);
	return it != m_resourceWriteStates.end() ? it->second : Hush::Graphics::EResourceState::Undefined;
}

void Hush::RenderGraph::RenderGraph::DependencyLevel::DetectMultiQueueReads()
{
	m_resourcesReadByMultipleQueues.clear();
	m_queuesInvolvedInCrossDependencies.clear();

	// Map: ResourceId -> set of queue indices that read it
	boost::unordered_flat_map<ResourceId, boost::unordered_flat_set<uint32_t>> resourceReaders;

	for (RenderPassNode *node : m_passNodes)
	{
		for (const ResourceId &rid : node->m_readResources)
		{
			resourceReaders[rid].insert(node->m_queueIndex);
		}
	}

	for (auto &[rid, queues] : resourceReaders)
	{
		if (queues.size() > 1)
		{
			m_resourcesReadByMultipleQueues.insert(rid);
			for (uint32_t q : queues)
			{
				m_queuesInvolvedInCrossDependencies.insert(q);
			}
		}
	}
}

Hush::RenderGraph::ResourceId Hush::RenderGraph::RenderGraph::BuildContext::Read(
	ResourceId id, Hush::Graphics::EResourceState desiredState)
{
	m_passNode.AddReadResource(id);
	m_passNode.SetReadState(id, desiredState);
	return id;
}

Hush::RenderGraph::ResourceId Hush::RenderGraph::RenderGraph::BuildContext::Write(
	ResourceId id, Hush::Graphics::EResourceState desiredState)
{
	m_passNode.AddWrittenResource(id);
	m_passNode.SetWriteState(id, desiredState);
	return id;
}

void Hush::RenderGraph::RenderGraph::BuildContext::SetCullingMode(RenderPassNode::EPassCullingMode cullMode)
{
	m_passNode.m_cullingMode = cullMode;
}

void Hush::RenderGraph::RenderGraph::Reset()
{
	m_passes.clear();
	m_adjacencyList.clear();
	m_topologicalOrderedNodes.clear();
	m_dependencyLevels.clear();
	m_importedResourceInitialStates.clear();
	m_state = ERenderGraphState::Dirty;
	m_nextResourceId = 1; // 0 is invalid
	m_resourceManager.Clear();
}
