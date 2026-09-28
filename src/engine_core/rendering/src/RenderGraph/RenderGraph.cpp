
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
#include <algorithm>

using namespace Hush::RenderGraph;

void RenderGraph::ClearCompiledData() noexcept
{
	m_adjacencyList.clear();
	m_topologicalOrderedNodes.clear();
	m_dependencyLevels.clear();
	m_detectedQueueCount = 1;
	for (auto &node : m_passes)
	{
		node.m_nodesToSync.clear();
		node.m_syncIndexSet.clear();
		node.m_syncSignalRequired = false;
		node.m_fenceSignalValue = 0;
		node.m_dependencyLevelIndex = 0;
		node.m_queueSequence = 0;
		node.m_scheduleIndex = 0;
	}
}

Hush::Result<void, EGraphError> RenderGraph::Compile(std::pmr::memory_resource &scratch)
{
	if (m_state == ERenderGraphState::Compiled)
	{
		return Hush::Success();
	}
	ClearCompiledData();
	if (m_buildError != EGraphError::None)
	{
		return m_buildError;
	}
	BuildAdjacencyList(scratch);
	if (!TopologicalSort(scratch))
	{
		ClearCompiledData();
		return EGraphError::Cycle;
	}
	BuildDependencyLevels();
	FinalizeDependencyLevels(scratch);
	CullRedundantSyncPoints();
	m_state = ERenderGraphState::Compiled;
	return Hush::Success();
}

void RenderGraph::BuildAdjacencyList(std::pmr::memory_resource &scratch)
{
	constexpr uint32_t none = std::numeric_limits<uint32_t>::max();
	struct History
	{
		explicit History(std::pmr::memory_resource &resource)
			: readers(&resource)
		{
		}
		uint32_t writer = none;
		std::pmr::vector<uint32_t> readers;
	};
	std::pmr::vector<History> histories{&scratch};
	histories.reserve(m_nextResourceId);
	for (uint32_t id = 0; id < m_nextResourceId; ++id)
	{
		histories.emplace_back(scratch);
	}

	// A producer can be discovered through several resources. Stamp once per
	// consumer rather than hashing each edge or searching its adjacency list.
	std::pmr::vector<uint32_t> seen(m_passes.size(), none, &scratch);
	m_adjacencyList.resize(m_passes.size());

	for (uint32_t consumer = 0; consumer < m_passes.size(); ++consumer)
	{
		auto &node = m_passes[consumer];
		auto addEdge = [&](uint32_t producer) {
			if (producer == none || producer == consumer || seen[producer] == consumer)
			{
				return;
			}
			seen[producer] = consumer;
			m_adjacencyList[producer].push_back(consumer);
			if (m_passes[producer].m_queueIndex != node.m_queueIndex)
			{
				m_passes[producer].SetHasCrossDependency(node);
			}
		};

		for (ResourceId id : node.m_readResources)
		{
			// A read-modify-write is ONE access to the preceding epoch.
			if (node.m_writtenResources.contains(id))
			{
				continue;
			}
			auto &history = histories[id.id];
			addEdge(history.writer);
			history.readers.push_back(consumer);
		}

		for (ResourceId id : node.m_writtenResources)
		{
			auto &history = histories[id.id];
			addEdge(history.writer);
			for (uint32_t reader : history.readers)
			{
				addEdge(reader);
			}
			history.readers.clear();
			history.writer = consumer;
		}
	}
}

bool RenderGraph::TopologicalSort(std::pmr::memory_resource &scratch)
{
	// Kahn traversal avoids recursion on long chains and retains cycle checking
	// if explicit dependency edges are added in the future.
	std::pmr::vector<uint32_t> indegree(m_passes.size(), &scratch);
	for (const auto &edges : m_adjacencyList)
	{
		for (uint32_t next : edges)
		{
			++indegree[next];
		}
	}
	m_topologicalOrderedNodes.reserve(m_passes.size());
	for (uint32_t i = 0; i < m_passes.size(); ++i)
	{
		if (indegree[i] == 0)
		{
			m_topologicalOrderedNodes.push_back(&m_passes[i]);
		}
	}
	for (size_t cursor = 0; cursor < m_topologicalOrderedNodes.size(); ++cursor)
	{
		for (uint32_t next : m_adjacencyList[m_topologicalOrderedNodes[cursor]->m_unorderedPassIndex])
		{
			if (--indegree[next] == 0)
			{
				m_topologicalOrderedNodes.push_back(&m_passes[next]);
			}
		}
	}
	return m_topologicalOrderedNodes.size() == m_passes.size();
}

void RenderGraph::BuildDependencyLevels()
{
	if (m_passes.empty())
	{
		return;
	}
	uint32_t maxLevel = 0;
	for (const auto *node : m_topologicalOrderedNodes)
	{
		for (uint32_t next : m_adjacencyList[node->m_unorderedPassIndex])
		{
			auto &level = m_passes[next].m_dependencyLevelIndex;
			level = std::max(level, node->m_dependencyLevelIndex + 1);
			maxLevel = std::max(maxLevel, level);
		}
	}
	m_dependencyLevels.resize(maxLevel + 1);
	for (auto &node : m_passes)
	{
		auto &level = m_dependencyLevels[node.m_dependencyLevelIndex];
		level.m_levelIndex = node.m_dependencyLevelIndex;
		level.AddNode(&node);
		m_detectedQueueCount = std::max(m_detectedQueueCount, node.m_queueIndex + 1);
	}
}

void RenderGraph::FinalizeDependencyLevels(std::pmr::memory_resource &scratch)
{
	std::array<uint64_t, PASS_TYPE_COUNT> sequence{};
	uint64_t scheduleIndex = 0;
	for (auto &level : m_dependencyLevels)
	{
		level.m_nodesPerQueue.resize(m_detectedQueueCount);
		for (auto *node : level.m_passNodes)
		{
			level.m_nodesPerQueue[node->m_queueIndex].push_back(node);
			node->m_queueSequence = ++sequence[node->m_queueIndex];
			node->m_scheduleIndex = ++scheduleIndex;
		}
		level.DetectMultiQueueReads(scratch);
	}
}

void RenderGraph::CullNodeSyncPoints(RenderPassNode &node, const RenderPassNode *previous) const
{
	auto &knowledge = node.m_syncIndexSet;
	knowledge.assign(m_detectedQueueCount, RenderPassNode::INVALID_SYNC_INDEX);
	if (previous != nullptr)
	{
		knowledge = previous->m_syncIndexSet;
	}
	// FIFO covers all earlier producers on a queue. At most three candidates
	// remain, regardless of the pass's original fan-in.
	std::array<RenderPassNode *, PASS_TYPE_COUNT> closest{};
	for (auto *dependency : node.m_nodesToSync)
	{
		auto *&candidate = closest[dependency->m_queueIndex];
		if (candidate == nullptr || candidate->m_queueSequence < dependency->m_queueSequence)
		{
			candidate = dependency;
		}
	}
	// Later scheduled dependencies may already cover earlier candidates.
	std::sort(closest.begin(), closest.end(), [](const auto *left, const auto *right) {
		return (left != nullptr ? left->m_scheduleIndex : 0) > (right != nullptr ? right->m_scheduleIndex : 0);
	});
	node.m_nodesToSync.clear();
	for (auto *dependency : closest)
	{
		if (dependency == nullptr || knowledge[dependency->m_queueIndex] >= dependency->m_queueSequence)
		{
			continue;
		}
		node.m_nodesToSync.push_back(dependency);
		dependency->m_syncSignalRequired = true;
		for (uint32_t q = 0; q < m_detectedQueueCount; ++q)
		{
			knowledge[q] = std::max(knowledge[q], dependency->m_syncIndexSet[q]);
		}
	}
	knowledge[node.m_queueIndex] = node.m_queueSequence;
}

void RenderGraph::CullRedundantSyncPoints()
{
	std::array<RenderPassNode *, PASS_TYPE_COUNT> previous{};
	for (auto &node : m_passes)
	{
		node.m_syncSignalRequired = false;
	}
	for (auto &level : m_dependencyLevels)
	{
		for (auto *node : level.m_passNodes)
		{
			CullNodeSyncPoints(*node, previous[node->m_queueIndex]);
			previous[node->m_queueIndex] = node;
		}
	}
	for (auto &level : m_dependencyLevels)
	{
		for (const auto *node : level.m_passNodes)
		{
			if (node->m_syncSignalRequired || !node->m_nodesToSync.empty())
			{
				level.m_queuesInvolvedInCrossDependencies.insert(node->m_queueIndex);
			}
		}
	}
}

bool RenderPassNode::WritesResource(ResourceId id) const
{
	return m_writtenResources.contains(id);
}

void RenderPassNode::SetHasCrossDependency(RenderPassNode &other)
{
	other.m_nodesToSync.push_back(this);
}

void RenderPassNode::Execute(Hush::Graphics::ICommandList *cmdList, ResourceManager &resourceManager)
{
	m_pass->Execute(cmdList, resourceManager);
}

void RenderPassNode::AddReadResource(ResourceId id)
{
	m_readResources.insert(id);
}
void RenderPassNode::AddWrittenResource(ResourceId id)
{
	m_writtenResources.insert(id);
}
void RenderPassNode::SetReadState(ResourceId id, Hush::Graphics::EResourceState state)
{
	m_resourceReadStates[id] = state;
}
Hush::Graphics::EResourceState RenderPassNode::GetReadState(ResourceId id) const
{
	auto it = m_resourceReadStates.find(id);
	return it != m_resourceReadStates.end() ? it->second : Hush::Graphics::EResourceState::Undefined;
}
Hush::Graphics::EResourceState RenderPassNode::GetWriteState(ResourceId id) const
{
	auto it = m_resourceWriteStates.find(id);
	return it != m_resourceWriteStates.end() ? it->second : Hush::Graphics::EResourceState::Undefined;
}

void RenderGraph::DependencyLevel::DetectMultiQueueReads(std::pmr::memory_resource &scratch)
{
	m_resourcesReadByMultipleQueues.clear();
	m_queuesInvolvedInSharedReads.clear();
	using Entry = std::pair<const ResourceId, uint32_t>;
	boost::unordered_flat_map<ResourceId, uint32_t, boost::hash<ResourceId>, std::equal_to<>,
							  std::pmr::polymorphic_allocator<Entry>>
		readers{std::pmr::polymorphic_allocator<Entry>{&scratch}};
	for (const auto *node : m_passNodes)
	{
		for (ResourceId id : node->m_readResources)
		{
			readers[id] |= 1U << node->m_queueIndex;
		}
	}
	for (auto [id, queues] : readers)
	{
		if ((queues & (queues - 1)) == 0)
		{
			continue;
		}
		m_resourcesReadByMultipleQueues.insert(id);
		for (uint32_t q = 0; q < PASS_TYPE_COUNT; ++q)
		{
			if ((queues & (1U << q)) != 0)
			{
				m_queuesInvolvedInSharedReads.insert(q);
			}
		}
	}
}

ResourceId RenderGraph::BuildContext::Read(ResourceId id, Hush::Graphics::EResourceState desiredState)
{
	if (m_renderGraph.m_resourceManager.GetResourceHandle(id) == nullptr)
	{
		m_renderGraph.m_buildError = EGraphError::InvalidResource;
		return {};
	}
	m_passNode.AddReadResource(id);
	m_passNode.SetReadState(id, desiredState);
	return id;
}

ResourceId RenderGraph::BuildContext::Write(ResourceId id, Hush::Graphics::EResourceState desiredState)
{
	if (m_renderGraph.m_resourceManager.GetResourceHandle(id) == nullptr)
	{
		m_renderGraph.m_buildError = EGraphError::InvalidResource;
		return {};
	}
	m_passNode.AddWrittenResource(id);
	m_passNode.SetWriteState(id, desiredState);
	return id;
}

void RenderGraph::BuildContext::SetCullingMode(RenderPassNode::EPassCullingMode cullMode)
{
	m_passNode.m_cullingMode = cullMode;
}

void RenderGraph::Reset()
{
	ClearCompiledData();
	m_passes.clear();
	m_importedResourceInitialStates.clear();
	m_state = ERenderGraphState::Dirty;
	m_buildError = EGraphError::None;
	m_nextResourceId = 1;
	m_resourceManager.Clear();
	m_executionContext.reset();
}
