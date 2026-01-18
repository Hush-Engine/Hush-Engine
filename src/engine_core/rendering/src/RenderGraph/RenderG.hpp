/*! \file RenderGraph.hpp
	\author Alan Ramirez Herrera
	\date 2025-11-17
	\brief RenderGraph implementation for rendering based on DAG scheduling
*/
#pragma once

#include <vector>
#include <list>
#include <memory>
#include <string>
#include <string_view>
#include <limits>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include "ResourceHandle.hpp"

namespace Hush::Exp::RenderGraph
{
    class IRenderContext;
    class ICommandQueue;

    struct ResourceId
    {
        uint32_t id = 0;

        bool operator==(const ResourceId& other) const { return id == other.id; }
        bool operator!=(const ResourceId& other) const { return id != other.id; }
    };
}

// Hash specialization for ResourceId
template <>
struct std::hash<Hush::Exp::RenderGraph::ResourceId>
{
    std::size_t operator()(const Hush::Exp::RenderGraph::ResourceId& rid) const noexcept
    {
        return std::hash<uint32_t>{}(rid.id);
    }
};

namespace Hush::Exp::RenderGraph
{
    /// Base class for all passes.
    ///
    /// This is not meant to be used directly by users but instead the Pass template.
    /// However, implementing this manually can be useful for language bindings.
    struct PassBase
    {
        PassBase() = default;
        virtual ~PassBase() = default;

        PassBase(const PassBase&) = delete;
        PassBase& operator=(const PassBase&) = delete;
        PassBase(PassBase&&) = delete;
        PassBase& operator=(PassBase&&) = delete;

        /// Executes the current pass with the given Render context.
        ///
        /// \param context Render context.
        virtual void Execute(void* context) = 0;
    };

    /// Templated pass implementation that stores PassData and execution callback
    template <typename PassData, typename ExecuteFn>
        requires(std::is_invocable_r_v<void, ExecuteFn, PassData &, void *>)
    class Pass : public PassBase
    {
    public:
        Pass(ExecuteFn &&e) : execFn(std::move(e)) {}

        void Execute(void *ctx) override
        {
            execFn(data, ctx);
        }

        ExecuteFn execFn;
        PassData data;
    };

    /// Represents a single pass inside of the render graph.
    ///
    /// A pass is a unit of work that reads and writes GPU resources.
    /// There are two execution contexts when creating a pass:
    /// - Build context: Used to declare resource usage (reads/writes) and create resources.
    /// - Execute context: Used to perform the actual rendering work.
    class RenderPassNode
    {
        friend class RenderGraph;

        static constexpr uint64_t INVALID_SYNC_INDEX = std::numeric_limits<uint64_t>::max();

    public:
        enum class EPassCullingMode : uint8_t
        {
            CullIfPossible,
            NeverCull,
        };

    private:
        RenderPassNode(std::string_view name, uint32_t nodeId, EPassType passType,
                      std::unique_ptr<PassBase> &&pass)
            : m_name(name),
              m_unorderedPassIndex(nodeId),
              m_pass(std::move(pass)),
              m_passType(passType),
              m_queueIndex(static_cast<uint32_t>(passType))
        {
        }

        /// Check if this pass writes to a resource
        [[nodiscard]]
        bool WritesResource(ResourceId id) const
        {
            return m_writtenResources.find(id) != m_writtenResources.end();
        }

        /// Set that this pass has a cross-dependency with another pass.
        void SetHasCrossDependency(RenderPassNode &other)
        {
            m_syncSignalRequired = true;
            other.m_nodesToSync.push_back(this);
        }

        void Execute(void* ctx)
        {
            m_pass->Execute(ctx);
        }

        void AddReadResource(ResourceId id)
        {
            m_readResources.insert(id);
        }

        void AddWrittenResource(ResourceId id)
        {
            m_writtenResources.insert(id);
        }

    private:
        boost::unordered_flat_set<ResourceId> m_readResources;
        boost::unordered_flat_set<ResourceId> m_writtenResources;
        std::string m_name;
        std::vector<uint64_t> m_syncIndexSet;
        std::vector<RenderPassNode*> m_nodesToSync;

        std::unique_ptr<PassBase> m_pass;
        EPassType m_passType;
        EPassCullingMode m_cullingMode = EPassCullingMode::CullIfPossible;

        uint32_t m_queueIndex = 0;
        uint32_t m_unorderedPassIndex = 0;
        uint32_t m_dependencyLevelIndex = 0;
        bool m_syncSignalRequired = false;
    };

    enum class EPassType : uint8_t
    {
        Graphics = 0,
        Compute = 1,
        Transfer = 2,
    };

    /// Render graph implementation based on DAG scheduling.
    ///
    /// Based on the article:
    /// https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
    class RenderGraph
    {
        friend class BuildContext;

        /// State of the render graph
        enum class ERenderGraphState : uint8_t
        {
            /// The graph is compiled and ready for execution
            Compiled,
            /// The graph is dirty and needs to be compiled
            Dirty,
        };

        /// Represents a dependency level - a group of passes that can execute in parallel
        class DependencyLevel
        {
        public:
            friend class RenderGraph;
            using NodeList = std::list<RenderPassNode*>;

        private:
            void AddNode(RenderPassNode* node)
            {
                m_passNodes.push_back(node);
            }

            RenderPassNode* RemoveNode(NodeList::iterator it)
            {
                RenderPassNode* node = *it;
                m_passNodes.erase(it);
                return node;
            }

        private:
            boost::unordered_flat_set<uint32_t> m_queuesInvolvedInCrossDependencies;
            boost::unordered_flat_set<ResourceId> m_resourcesReadByMultipleQueues;
            std::vector<std::vector<RenderPassNode*>> m_nodesPerQueue;

            uint32_t m_levelIndex{};
            NodeList m_passNodes;
        };

    public:
        /// Allows creating resources and declaring usage (read/write).
        class BuildContext
        {
        public:
            /// Read a resource inside of the pass
            ResourceId Read(ResourceId id)
            {
                m_passNode.AddReadResource(id);
                return id;
            }

            /// Write a resource inside of the pass
            ResourceId Write(ResourceId id)
            {
                m_passNode.AddWrittenResource(id);
                return id;
            }

            /// Create a resource inside of the pass
            template <Hush::RenderGraph::ResourceConcept T>
            ResourceId Create(std::string_view name, const typename T::Descriptor &&desc)
            {
                // TODO: Implement resource creation when resource management is added
                ResourceId id{static_cast<uint32_t>(m_renderGraph.m_nextResourceId++)};
                m_passNode.AddWrittenResource(id);
                return id;
            }

            /// Set the culling mode for this pass
            void SetCullingMode(RenderPassNode::EPassCullingMode cullMode)
            {
                m_passNode.m_cullingMode = cullMode;
            }

        private:
            friend class RenderGraph;

            BuildContext(RenderGraph &renderGraph, RenderPassNode &passNode)
                : m_renderGraph(renderGraph),
                  m_passNode(passNode)
            {
            }

        private:
            RenderGraph &m_renderGraph;
            RenderPassNode &m_passNode;
        };

    public:
        RenderGraph() = default;

        /// Add a pass with typed local data and the two callbacks (build, execute)
        template <typename PassData, typename BuildFn, typename ExecuteFn>
            requires(std::is_invocable_r_v<void, BuildFn, BuildContext &, PassData &> &&
                     std::is_invocable_r_v<void, ExecuteFn, PassData &, void *>)
        const PassData& AddPass(EPassType passType, std::string_view name, BuildFn &&buildFn, ExecuteFn &&execFn)
        {
            // Create the pass execution context
            auto pass = std::make_unique<Pass<PassData, ExecuteFn>>(std::forward<ExecuteFn>(execFn));
            auto &passData = pass->data;

            const auto passNodeId = static_cast<uint32_t>(m_passes.size());

            // Create the pass node
            m_passes.emplace_back(name, passNodeId, passType, std::move(pass));
            RenderPassNode &passNode = m_passes.back();

            // Invoke build callback immediately
            BuildContext buildCtx(*this, passNode);
            std::invoke(std::forward<BuildFn>(buildFn), buildCtx, passData);

            // Mark graph as dirty
            m_state = ERenderGraphState::Dirty;

            return passData;
        }

        /// Compile the render graph, optimizing and culling passes as needed.
        void Compile();

        /// Execute the render graph.
        void Execute(void *ctx);

        /// Check if the render graph is dirty and needs recompilation.
        [[nodiscard]]
        bool IsDirty() const noexcept
        {
            return m_state == ERenderGraphState::Dirty;
        }

        /// Reset the graph to allow rebuilding
        void Reset()
        {
            m_passes.clear();
            m_adjacencyList.clear();
            m_topologicalOrderedNodes.clear();
            m_dependencyLevels.clear();
            m_state = ERenderGraphState::Dirty;
            m_nextResourceId = 1; // 0 is invalid
        }

    private:
        /// Builds the adjacency list representing pass dependencies.
        void BuildAdjacencyList();

        /// Depth-first search for topological sorting and cycle detection
        void DFS(uint32_t nodeIndex, std::vector<bool>& visited,
                std::vector<bool> &onStack, bool& isCyclic);

        /// Topological sort of the graph
        void TopologicalSort();

        /// Build dependency levels using longest path algorithm
        void BuildDependencyLevels();

        /// Finalize dependency levels by organizing passes per queue
        void FinalizeDependencyLevels();

        /// Cull redundant synchronization points using SSIS algorithm
        void CullRedundantSyncPoints();

    private:
        /// List of passes in the render graph
        std::vector<RenderPassNode> m_passes;

        /// Adjacency list representing pass dependencies
        std::vector<std::vector<uint32_t>> m_adjacencyList;

        /// Topologically sorted passes
        std::vector<RenderPassNode*> m_topologicalOrderedNodes;

        /// Dependency levels for parallel execution
        std::vector<DependencyLevel> m_dependencyLevels;

        ERenderGraphState m_state = ERenderGraphState::Dirty;
        uint32_t m_detectedQueueCount = 1;
        uint32_t m_nextResourceId = 1;
    };
}
