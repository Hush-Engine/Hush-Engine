#include "RenderG.hpp"
#include <Assertions.hpp>

Hush::Exp::RenderGraph::RenderPassNode::RenderPassNode(std::string_view name)
    : m_name(name)
{
}

bool Hush::Exp::RenderGraph::RenderPassNode::WritesResource(ResourceId id) const
{
    return m_writtenResources.find(id) != m_writtenResources.end();
}

void Hush::Exp::RenderGraph::RenderPassNode::SetHasCrossDependency(RenderPassNode &other)
{
    m_syncSignalRequired = true;
    other.m_nodesToSync.push_back(this);
}

void Hush::Exp::RenderGraph::RenderGraph::Compile()
{
    BuildAdjacencyList();
    TopologicalSort();
    BuildDependencyLevels();
    FinalizeDependencyLevels();
    CullRedundantSyncPoints();
}

void Hush::Exp::RenderGraph::RenderGraph::BuildAdjacencyList()
{
    m_adjacencyList.resize(m_passes.size());

    for (uint32_t nodeIndex = 0; nodeIndex < m_passes.size(); ++nodeIndex)
    {
        RenderPassNode& passNode = m_passes[nodeIndex];
		std::vector<uint32_t> &adjacentNodeIndices = m_adjacencyList[nodeIndex];

		for (uint32_t otherNodeIndex = 0; otherNodeIndex < m_passes.size(); ++otherNodeIndex)
        {
            if (nodeIndex == otherNodeIndex)
            {
                continue; // Skip self-dependency
            }

            RenderPassNode& otherPassNode = m_passes[otherNodeIndex];

            // We need to check if any of the other node reads a resource written by the current node.
            // If so, we have a dependency between passNode -> otherPassNode
            for (const ResourceId& otherReadResources : otherPassNode.m_readResources)
    		{
                // This means that otherPassNode depends on passNode
                if (passNode.WritesResource(otherReadResources))
                {
                    adjacentNodeIndices.push_back(otherNodeIndex);

                    if (otherPassNode.m_queueIndex != passNode.m_queueIndex)
                    {
                        // We have a cross-queue dependency!
                        // This means that we need to insert an explicit synchronization point between the two passes.
                        passNode.SetHasCrossDependency(otherPassNode);
                    }

                    break; // No need to check other resources for this otherPassNode
                }
            }
        }
	}
}

void Hush::Exp::RenderGraph::RenderGraph::DFS(uint32_t nodeIndex, std::vector<bool>& visited, std::vector<bool> &onStack, bool& isCyclic)
{
    if (isCyclic)
    {
        return;
    }

    visited[nodeIndex] = true;
    onStack[nodeIndex] = true;

    for (uint32_t neighborIndex : m_adjacencyList[nodeIndex])
    {
        if (visited[neighborIndex] && onStack[nodeIndex])
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

    m_topologicalOrderedNodes.push_back(&m_passes[nodeIndex]);
}

void Hush::Exp::RenderGraph::RenderGraph::TopologicalSort()
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

            HUSH_ASSERT(isCyclic, "Render graph contains cycles! Cannot proceed with execution.");
        }
    }

    std::reverse(m_topologicalOrderedNodes.begin(), m_topologicalOrderedNodes.end());
}

void Hush::Exp::RenderGraph::RenderGraph::BuildDependencyLevels()
{
    std::vector<int32_t> longestPathLengths(m_topologicalOrderedNodes.size(), 0);

    uint32_t currentLevelIndex = 1;

    for (uint32_t nodeIndex = 0; nodeIndex < m_topologicalOrderedNodes.size(); ++nodeIndex)
    {
        const uint32_t originalIndex = m_topologicalOrderedNodes[nodeIndex]->m_unorderedPassIndex;
        const uint32_t adjacencyListIndex = originalIndex;

       for (auto adjacentNodeIndex : m_adjacencyList[adjacencyListIndex])
       {
            if (longestPathLengths[adjacentNodeIndex] < longestPathLengths[nodeIndex] + 1)
            {
                int32_t newLongestPathLength = longestPathLengths[nodeIndex] + 1;
                longestPathLengths[adjacentNodeIndex] = newLongestPathLength;
                // +1 since levels are 1-indexed
                currentLevelIndex = std::max(currentLevelIndex, static_cast<uint32_t>(newLongestPathLength + 1));
            }
       }

       m_dependencyLevels.resize(currentLevelIndex);
       m_detectedQueueCount = 1;

       for (uint32_t i = 0; i < m_topologicalOrderedNodes.size(); ++i)
       {
            RenderPassNode& node = m_passes[nodeIndex];
            const auto levelIndex = static_cast<uint32_t>(longestPathLengths[i]);

            DependencyLevel& level = m_dependencyLevels[levelIndex];
            level.m_levelIndex = levelIndex;
            level.AddNode(&node);

            node.m_dependencyLevelIndex = levelIndex;

            m_detectedQueueCount = std::max(m_detectedQueueCount, node.m_queueIndex + 1);
       }
    }
}

void Hush::Exp::RenderGraph::RenderGraph::CullRedundantSynchronizations()
{
    for (RenderPassNode& node : m_passes)
    {
        node.m_syncIndexSet.resize(m_detectedQueueCount, RenderPassNode::INVALID_SYNC_INDEX);
    }

    for (DependencyLevel& level : m_dependencyLevels)
    {
        // For the first pass, we need to find the closest sync point to sync and compute the sufficient sync index set.
        // for each pass in the level
        for (RenderPassNode *passNode : level.m_passNodes)
        {
            std::vector<const RenderPassNode*> nodesToSync{m_detectedQueueCount, nullptr};

            for (RenderPassNode* dependencyNode : passNode->m_nodesToSync)
            {
                const RenderPassNode* closestSyncNode = nodesToSync[dependencyNode->m_queueIndex];

                // TODO: a
                if (closestSyncNode == nullptr ||
                    dependencyNode->m_dependencyLevelIndex > closestSyncNode->m_dependencyLevelIndex)
                {
                    nodesToSync[dependencyNode->m_queueIndex] = dependencyNode;
                }
            }

            passNode->m_nodesToSync.clear();

        }
    }

}
