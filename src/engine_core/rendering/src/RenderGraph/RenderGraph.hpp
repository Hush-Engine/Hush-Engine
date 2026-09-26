/*! \file RenderGraph.hpp
	\author Alan Ramirez Herrera
	\date 2025-11-17
	\brief RenderGraph implementation for rendering based on DAG scheduling.

	The RenderGraph is a pure data structure representing the dependency graph
	of render passes. It handles:
	  - Pass registration (AddPass) with typed build/execute callbacks
	  - Resource declaration (create, import, read, write) during build phase
	  - Graph compilation (topological sort, dependency levels, SSIS)

	It does NOT handle execution, synchronization, barriers, or any GPU
	interaction. That responsibility belongs to RenderGraphExecutor.
*/
#pragma once

#include <array>
#include <boost/unordered/unordered_flat_map_fwd.hpp>
#include <vector>
#include <list>
#include <memory>
#include <memory_resource>
#include <string>
#include <string_view>
#include <limits>
#include <boost/unordered/unordered_flat_map.hpp>
#include <boost/unordered/unordered_flat_set.hpp>
#include "RHI/GraphicsTypes.hpp"
#include "ResourceHandle.hpp"
#include "Result.hpp"

namespace Hush::Graphics
{
	class ICommandList;
	class ICopyCommandList;
	class IComputeCommandList;
	class IGraphicsCommandList;
	class IGraphicsDevice;
} // namespace Hush::Graphics

namespace Hush::RenderGraph
{
	class IRenderContext;
	class ICommandQueue;

	struct ResourceId
	{
		uint32_t id = 0;

		bool operator==(const ResourceId &other) const
		{
			return id == other.id;
		}
		bool operator!=(const ResourceId &other) const
		{
			return id != other.id;
		}
	};
} // namespace Hush::RenderGraph

// Hash specialization for ResourceId
template <>
struct std::hash<Hush::RenderGraph::ResourceId>
{
	std::size_t operator()(const Hush::RenderGraph::ResourceId &rid) const noexcept
	{
		return std::hash<uint32_t>{}(rid.id);
	}
};

// Hash for boost
template <>
struct boost::hash<Hush::RenderGraph::ResourceId>
{
	std::size_t operator()(const Hush::RenderGraph::ResourceId &rid) const noexcept
	{
		return std::hash<uint32_t>{}(rid.id);
	}
};

namespace Hush::RenderGraph
{
	enum class EGraphError : uint8_t
	{
		None,
		InvalidResource,
		InvalidQueue,
		QueueMapLocked,
		DuplicateResource,
		NotCompiled,
		InvalidState,
		UnsupportedTransition,
		DeviceFailure,
		Cycle,
	};

	/// Manages logical resources within the render graph, including creation,
	/// tracking, and resolution.
	///
	/// @note Resources declared here are logical: the ResourceHandle holds a
	/// descriptor and (for transient resources) a default-constructed resource
	/// instance. Actual GPU allocation is deferred to the executor's realization
	/// phase (ResourceHandle::CreateResource). For imported resources, the GPU
	/// object is provided at declaration time and is already realized.
	class ResourceManager
	{
	public:
		ResourceManager() = default;
		~ResourceManager() = default;

		ResourceManager(const ResourceManager &) = delete;
		ResourceManager &operator=(const ResourceManager &) = delete;
		ResourceManager(ResourceManager &&) = delete;
		ResourceManager &operator=(ResourceManager &&) = delete;

		void AddResource(std::string name, ResourceId id, Hush::RenderGraph::ResourceHandle handle)
		{
			m_resources.emplace(id, std::move(handle));
			m_resourceNameToId.emplace(std::move(name), id);
		}

		[[nodiscard]]
		Hush::RenderGraph::ResourceHandle *GetResourceHandle(const ResourceId &id) const
		{
			auto it = m_resources.find(id);
			if (it != m_resources.end())
			{
				return &it->second;
			}
			return nullptr;
		}

		[[nodiscard]]
		Hush::RenderGraph::ResourceHandle *GetResourceHandle(std::string_view name) const
		{
			auto it = m_resourceNameToId.find(std::string(name));
			if (it != m_resourceNameToId.end())
			{
				return GetResourceHandle(it->second);
			}

			return nullptr;
		}

		[[nodiscard]]
		ResourceId GetResourceId(std::string_view name) const
		{
			auto it = m_resourceNameToId.find(std::string(name));
			if (it != m_resourceNameToId.end())
			{
				return it->second;
			}

			return ResourceId{0};
		}

		template <typename T>
		[[nodiscard]]
		T *GetResource(const ResourceId &id) const
		{
			auto *handle = GetResourceHandle(id);
			if (handle != nullptr)
			{
				return &handle->GetResourceInstance<T>();
			}
			return nullptr;
		}

		/// Update the resource instance of an existing imported (External) handle.
		///
		/// This swaps the underlying data (e.g. swapchain backbuffer pointer)
		/// without modifying graph topology, resource IDs, or compilation state.
		///
		/// @tparam T Must satisfy ResourceConcept and match the type used at import time.
		/// @param id The ResourceId of the imported resource to update.
		/// @param newResource The replacement resource instance.
		template <ResourceConcept T>
		[[nodiscard]]
		bool UpdateResource(const ResourceId &id, T &&newResource)
		{
			auto *handle = GetResourceHandle(id);
			return handle != nullptr && handle->UpdateExternalResource<T>(std::forward<T>(newResource));
		}

		/// Iterate over all resource handles (used by the executor for realization)
		template <typename Fn>
		void ForEachResource(Fn &&fn) const
		{
			for (auto &[id, handle] : m_resources)
			{
				std::forward<Fn>(fn)(id, handle);
			}
		}

		/// Get the total number of resources in the manager.
		[[nodiscard]]
		size_t GetResourceCount() const noexcept
		{
			return m_resources.size();
		}

		void Clear()
		{
			m_resources.clear();
			m_resourceNameToId.clear();
		}

	private:
		mutable boost::unordered_flat_map<ResourceId, Hush::RenderGraph::ResourceHandle> m_resources;
		mutable boost::unordered_flat_map<std::string, ResourceId> m_resourceNameToId;
	};

	/// Base class for all passes.
	///
	/// This is not meant to be used directly by users but instead the Pass template.
	/// However, implementing this manually can be useful for language bindings.
	struct PassBase
	{
		PassBase() = default;
		virtual ~PassBase() = default;

		PassBase(const PassBase &) = delete;
		PassBase &operator=(const PassBase &) = delete;
		PassBase(PassBase &&) = delete;
		PassBase &operator=(PassBase &&) = delete;

		/// Executes the current pass with the given command list.
		///
		/// @param commandList Command list appropriate for this pass type.
		/// @param resourceManager Provides access to resources declared in the graph.
		virtual void Execute(Hush::Graphics::ICommandList *commandList, const ResourceManager &resourceManager) = 0;
	};

	/// Templated pass implementation that stores PassData and execution callback
	template <typename PassData, typename ExecuteFn>
		requires(std::is_invocable_r_v<void, ExecuteFn, PassData &, Hush::Graphics::ICommandList *, ResourceManager &>)
	class Pass : public PassBase
	{
	public:
		Pass(ExecuteFn &&e)
			: execFn(std::move(e))
		{
		}

		void Execute(Hush::Graphics::ICommandList *cmdList, const ResourceManager &resourceManager) override
		{
			execFn(data, cmdList, resourceManager);
		}

		ExecuteFn execFn;
		PassData data;
	};

	enum class EPassType : uint8_t
	{
		Graphics = 0,
		Compute = 1,
		Transfer = 2,
	};

	/// Maps EPassType to Graphics::EQueueType
	inline Hush::Graphics::EQueueType PassTypeToQueueType(EPassType passType)
	{
		switch (passType)
		{
		case EPassType::Compute:
			return Hush::Graphics::EQueueType::Compute;
		case EPassType::Transfer:
			return Hush::Graphics::EQueueType::Transfer;
		case EPassType::Graphics:
		default:
			return Hush::Graphics::EQueueType::Graphics;
		}
	}

	/// Represents a single pass inside of the render graph.
	///
	/// A pass is a unit of work that reads and writes GPU resources.
	/// There are two execution contexts when creating a pass:
	/// - Build context: Used to declare resource usage (reads/writes) and create resources.
	/// - Execute context: Used to perform the actual rendering work.
	class RenderPassNode
	{
		friend class RenderGraph;
		friend class RenderGraphExecutor;

		static constexpr uint64_t INVALID_SYNC_INDEX = 0;

	public:
		enum class EPassCullingMode : uint8_t
		{
			CullIfPossible,
			NeverCull,
		};

		RenderPassNode(std::string_view name, uint32_t nodeId, EPassType passType, std::shared_ptr<PassBase> &&pass,
					   uint32_t queueIndex = std::numeric_limits<uint32_t>::max())
			: m_name(name),
			  m_pass(std::move(pass)),
			  m_passType(passType),
			  m_queueIndex(queueIndex != std::numeric_limits<uint32_t>::max() ? queueIndex
																			  : static_cast<uint32_t>(passType)),
			  m_unorderedPassIndex(nodeId)
		{
		}

		~RenderPassNode() = default;
		RenderPassNode(const RenderPassNode &) = delete;
		RenderPassNode &operator=(const RenderPassNode &) = delete;
		RenderPassNode(RenderPassNode &&) noexcept = default;
		RenderPassNode &operator=(RenderPassNode &&) noexcept = default;

		[[nodiscard]]
		std::shared_ptr<const void> GetLifetimeToken() const
		{
			return m_pass;
		}

		[[nodiscard]]
		std::string_view GetName() const noexcept
		{
			return m_name;
		}

		[[nodiscard]]
		EPassType GetPassType() const noexcept
		{
			return m_passType;
		}

		[[nodiscard]]
		uint32_t GetQueueIndex() const noexcept
		{
			return m_queueIndex;
		}

		[[nodiscard]]
		uint32_t GetUnorderedIndex() const noexcept
		{
			return m_unorderedPassIndex;
		}

		[[nodiscard]]
		uint32_t GetDependencyLevelIndex() const noexcept
		{
			return m_dependencyLevelIndex;
		}

		[[nodiscard]]
		bool IsSyncSignalRequired() const noexcept
		{
			return m_syncSignalRequired;
		}

		[[nodiscard]]
		const boost::unordered_flat_set<ResourceId> &GetReadResources() const noexcept
		{
			return m_readResources;
		}

		[[nodiscard]]
		const boost::unordered_flat_set<ResourceId> &GetWrittenResources() const noexcept
		{
			return m_writtenResources;
		}

		[[nodiscard]]
		const std::vector<RenderPassNode *> &GetNodesToSync() const noexcept
		{
			return m_nodesToSync;
		}

		[[nodiscard]]
		const std::vector<uint64_t> &GetSyncIndexSet() const noexcept
		{
			return m_syncIndexSet;
		}

	private:
		/// Check if this pass writes to a resource
		[[nodiscard]]
		bool WritesResource(ResourceId id) const;

		/// Set that this pass has a cross-dependency with another pass.
		void SetHasCrossDependency(RenderPassNode &other);

		void Execute(Hush::Graphics::ICommandList *cmdList, ResourceManager &resourceManager);

		void AddReadResource(ResourceId id);

		void AddWrittenResource(ResourceId id);

		/// Declare the desired resource state for a read operation in this pass.
		void SetReadState(ResourceId id, Hush::Graphics::EResourceState state);

		/// Declare the desired resource state for a write operation in this pass.
		void SetWriteState(ResourceId id, Hush::Graphics::EResourceState state)
		{
			m_resourceWriteStates[id] = state;
		}

		/// Get the state this pass expects a resource to be in for reading.
		[[nodiscard]]
		Hush::Graphics::EResourceState GetReadState(ResourceId id) const;

		/// Get the state this pass expects a resource to be in for writing.
		[[nodiscard]]
		Hush::Graphics::EResourceState GetWriteState(ResourceId id) const;

	private:
		boost::unordered_flat_set<ResourceId> m_readResources;
		boost::unordered_flat_set<ResourceId> m_writtenResources;

		/// Desired resource states for reads and writes declared during build phase.
		boost::unordered_flat_map<ResourceId, Hush::Graphics::EResourceState> m_resourceReadStates;
		boost::unordered_flat_map<ResourceId, Hush::Graphics::EResourceState> m_resourceWriteStates;

		std::string m_name;

		/// SSIS (Sufficient Synchronization Index Set) — per-queue closest
		/// dependency indices, populated during CullRedundantSyncPoints().
		std::vector<uint64_t> m_syncIndexSet;

		/// Nodes this pass must wait on (cross-queue dependencies, after SSIS culling).
		std::vector<RenderPassNode *> m_nodesToSync;

		std::shared_ptr<PassBase> m_pass;
		EPassType m_passType;
		EPassCullingMode m_cullingMode = EPassCullingMode::CullIfPossible;

		uint32_t m_queueIndex = 0;
		uint32_t m_unorderedPassIndex = 0;
		uint32_t m_dependencyLevelIndex = 0;
		uint64_t m_queueSequence = 0;
		uint64_t m_scheduleIndex = 0;

		/// True if this node has a cross-queue dependency that requires a fence signal.
		bool m_syncSignalRequired = false;

		/// The fence value this node should signal after execution (set by executor during batching).
		uint64_t m_fenceSignalValue = 0;
	};

	/// Render graph implementation based on DAG scheduling.
	///
	/// Based on the article:
	/// https://levelup.gitconnected.com/organizing-gpu-work-with-directed-acyclic-graphs-f3fd5f2c2af3
	///
	/// This class is a **pure data structure** representing the dependency graph
	/// of render passes. It owns passes, computes the optimal execution order
	/// (topological sort), groups passes into dependency levels, and determines
	/// the minimal set of synchronization points (SSIS algorithm).
	///
	/// It does NOT interact with the GPU in any way. All execution, barrier
	/// computation, fence management, and command list batching are handled by
	/// RenderGraphExecutor. This separation allows:
	///   - Testing the graph without a GPU device
	///   - Swapping execution strategies without changing the graph
	///   - Clear ownership: RenderDevice owns both the graph and the executor
	class RenderGraph
	{
		friend class BuildContext;
		friend class RenderGraphExecutor;

		/// State of the render graph
		enum class ERenderGraphState : uint8_t
		{
			/// The graph is compiled and ready for execution
			Compiled,
			/// The graph is dirty and needs to be compiled
			Dirty,
		};

	public:
		/// @brief Sync token name for the resource upload system.
		///       Passes that consume uploaded resources should Read this resource in their build callback to create a
		///       dependency edge on the Transfer pass.
		static constexpr std::string_view RESOURCE_UPLOAD_SYNC_TOKEN_NAME = "Hush__UploadSyncToken__";

		/// Represents a dependency level — a group of passes that can execute in parallel.
		///
		/// Passes within a single dependency level share the same maximum
		/// recursion depth (longest path from root). They have no dependencies
		/// on each other and can be executed in arbitrary order, which enables
		/// parallel work execution across queues.
		class DependencyLevel
		{
		public:
			friend class RenderGraph;
			friend class RenderGraphExecutor;
			using NodeList = std::list<RenderPassNode *>;

			[[nodiscard]]
			const NodeList &GetPassNodes() const noexcept
			{
				return m_passNodes;
			}

			[[nodiscard]]
			const std::vector<std::vector<RenderPassNode *>> &GetNodesPerQueue() const noexcept
			{
				return m_nodesPerQueue;
			}

			[[nodiscard]]
			const boost::unordered_flat_set<uint32_t> &GetQueuesInvolvedInCrossDependencies() const noexcept
			{
				return m_queuesInvolvedInCrossDependencies;
			}

			[[nodiscard]]
			const boost::unordered_flat_set<uint32_t> &GetQueuesInvolvedInSharedReads() const noexcept
			{
				return m_queuesInvolvedInSharedReads;
			}

			[[nodiscard]]
			const boost::unordered_flat_set<ResourceId> &GetResourcesReadByMultipleQueues() const noexcept
			{
				return m_resourcesReadByMultipleQueues;
			}

			[[nodiscard]]
			uint32_t GetLevelIndex() const noexcept
			{
				return m_levelIndex;
			}

		private:
			void AddNode(RenderPassNode *node)
			{
				m_passNodes.push_back(node);
			}

			RenderPassNode *RemoveNode(NodeList::iterator it)
			{
				RenderPassNode *node = *it;
				m_passNodes.erase(it);
				return node;
			}

			/// Detect which resources are read by more than one queue in this level.
			/// Populates m_resourcesReadByMultipleQueues and
			/// m_queuesInvolvedInCrossDependencies.
			void DetectMultiQueueReads(std::pmr::memory_resource &scratch);

		private:
			boost::unordered_flat_set<uint32_t> m_queuesInvolvedInCrossDependencies;
			boost::unordered_flat_set<uint32_t> m_queuesInvolvedInSharedReads;
			boost::unordered_flat_set<ResourceId> m_resourcesReadByMultipleQueues;
			std::vector<std::vector<RenderPassNode *>> m_nodesPerQueue;

			uint32_t m_levelIndex{};
			NodeList m_passNodes;
		};

		/// Allows creating resources and declaring usage (read/write) during the
		/// build phase of a pass.
		///
		/// Resource creation is **deferred**: Create() stores a descriptor and
		/// a default-constructed resource instance but does NOT allocate the GPU
		/// resource. The executor calls ResourceHandle::CreateResource() during
		/// its realization step before execution begins.
		class BuildContext
		{
		public:
			/// Read a resource inside of the pass
			ResourceId Read(ResourceId id, Hush::Graphics::EResourceState desiredState =
											   Hush::Graphics::EResourceState::AnyShaderAccess);

			/// Write a resource inside of the pass
			ResourceId Write(ResourceId id, Hush::Graphics::EResourceState desiredState =
												Hush::Graphics::EResourceState::UnorderedAccess);

			/// Declare a transient resource. Does not declare an access; call Read/Write explicitly.
			///
			/// The resource descriptor is stored and a default-constructed resource
			/// instance is created, but actual GPU allocation is deferred until the
			/// executor's realization phase. This allows the graph to be built
			/// without any device dependency.
			template <Hush::RenderGraph::ResourceConcept T>
			ResourceId Create([[maybe_unused]] std::string_view name,
							  [[maybe_unused]] const typename T::Descriptor &&desc)
			{
				if (m_renderGraph.m_resourceManager.GetResourceId(name).id != 0)
				{
					m_renderGraph.m_buildError = EGraphError::DuplicateResource;
					return {};
				}
				ResourceId id{m_renderGraph.m_nextResourceId++};

				// Create the handle with a default-constructed resource — NOT realized yet.
				// The executor will call handle.CreateResource(device) before execution.
				Hush::RenderGraph::ResourceHandle handle = Hush::RenderGraph::ResourceHandle(
					desc, T{}, Hush::RenderGraph::ResourceHandle::EHandleType::Transient, id.id);

				m_renderGraph.m_resourceManager.AddResource(std::string(name), id, std::move(handle));
				return id;
			}

			/// Declare an external resource (e.g. swapchain image), without an implicit read.
			/// Call Read/Write explicitly. Producers precede consumers in registration order.
			///
			/// Imported resources are considered already realized because they are
			/// created and managed outside the graph. The executor will NOT call
			/// CreateResource() on these.
			///
			/// @tparam T Must satisfy ResourceConcept
			/// @param name Debug name for the resource
			/// @param externalResource The actual resource instance created outside the graph
			/// @param initialState The current resource state of the imported resource
			/// @return ResourceId that can be used to reference this resource in the graph
			template <Hush::RenderGraph::ResourceConcept T>
			ResourceId Import([[maybe_unused]] std::string_view name, T &&externalResource,
							  Hush::Graphics::EResourceState initialState = Hush::Graphics::EResourceState::Undefined)
			{
				if (m_renderGraph.m_resourceManager.GetResourceId(name).id != 0)
				{
					m_renderGraph.m_buildError = EGraphError::DuplicateResource;
					return {};
				}
				ResourceId id{m_renderGraph.m_nextResourceId++};

				Hush::RenderGraph::ResourceHandle handle =
					Hush::RenderGraph::ResourceHandle(typename T::Descriptor{}, std::forward<T>(externalResource),
													  Hush::RenderGraph::ResourceHandle::EHandleType::External, id.id);

				m_renderGraph.m_resourceManager.AddResource(std::string(name), id, std::move(handle));

				// Store the initial state for the executor to pick up later.
				m_renderGraph.m_importedResourceInitialStates[id] = initialState;

				return id;
			}

			/// Set the culling mode for this pass
			void SetCullingMode(RenderPassNode::EPassCullingMode cullMode);

			[[nodiscard]]
			ResourceId GetResourceIdByName(std::string_view name) const
			{
				const auto id = m_renderGraph.m_resourceManager.GetResourceId(name);
				if (id.id == 0)
				{
					m_renderGraph.m_buildError = EGraphError::InvalidResource;
				}
				return id;
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
		/// Construct a render graph.
		///
		/// Note: the graph itself has no device dependency. The device is only
		/// needed by the executor at execution time.

		/// Number of distinct EPassType values (Graphics, Compute, Transfer).
		static constexpr uint32_t PASS_TYPE_COUNT = 3;

		RenderGraph() = default;

		/// Set the physical queue index for a given logical pass type.
		///
		/// Call this once after construction (typically from RenderDevice) to
		/// inform the graph how pass types map to physical queues.  On
		/// multi-queue backends the default mapping (Graphics=0, Compute=1,
		/// Transfer=2) is used.  Single-queue backends (WebGPU) should set
		/// all entries to 0.
		[[nodiscard]]
		Hush::Result<void, EGraphError> SetQueueMap(EPassType passType, uint32_t queueIndex)
		{
			if (!m_passes.empty())
			{
				return EGraphError::QueueMapLocked;
			}
			if (static_cast<uint32_t>(passType) >= PASS_TYPE_COUNT || queueIndex >= PASS_TYPE_COUNT)
			{
				return EGraphError::InvalidQueue;
			}
			m_queueMap[static_cast<uint32_t>(passType)] = queueIndex;
			return Hush::Success();
		}

		/// Get the physical queue index for a given logical pass type.
		[[nodiscard]]
		uint32_t GetMappedQueueIndex(EPassType passType) const noexcept
		{
			return m_queueMap[static_cast<uint32_t>(passType)];
		}

		/// Add a pass with typed local data and the two callbacks (build, execute)
		template <typename PassData, typename BuildFn, typename ExecuteFn>
			requires(
				std::is_invocable_r_v<void, BuildFn, BuildContext &, PassData &> &&
				std::is_invocable_r_v<void, ExecuteFn, PassData &, Hush::Graphics::ICommandList *, ResourceManager &>)
		const PassData &AddPass(EPassType passType, std::string_view name, BuildFn &&buildFn, ExecuteFn &&execFn)
		{
			// Create the pass execution context
			auto pass = std::make_shared<Pass<PassData, ExecuteFn>>(std::forward<ExecuteFn>(execFn));
			auto &passData = pass->data;

			const auto passNodeId = static_cast<uint32_t>(m_passes.size());

			// Map the logical pass type to the physical queue index using the
			// device-provided lookup table.  Single-queue backends like WebGPU
			// collapse all types to queue 0, avoiding unnecessary cross-queue
			// synchronisation.
			const auto typeIndex = static_cast<uint32_t>(passType);
			if (typeIndex >= PASS_TYPE_COUNT)
			{
				m_buildError = EGraphError::InvalidQueue;
			}
			const uint32_t queueIndex = typeIndex < PASS_TYPE_COUNT ? m_queueMap[typeIndex] : 0;

			// Compiled pointers must be discarded BEFORE vector reallocation.
			Invalidate();

			// Create the pass node
			m_passes.emplace_back(name, passNodeId, passType, std::move(pass), queueIndex);
			RenderPassNode &passNode = m_passes.back();

			// Invoke build callback immediately
			BuildContext buildCtx(*this, passNode);
			std::invoke(std::forward<BuildFn>(buildFn), buildCtx, passData);

			// Mark graph as dirty
			m_state = ERenderGraphState::Dirty;

			return passData;
		}

		/// Compile the render graph, optimizing and culling passes as needed.
		/// Scratch allocations are destroyed before returning (also on failure).
		/// The caller may use the engine's frame resource or a graph-owner's pool;
		/// cached graph storage and pass/resource lifetimes never use this resource.
		[[nodiscard]]
		Hush::Result<void, EGraphError> Compile(std::pmr::memory_resource &scratch = *std::pmr::get_default_resource());

		/// Check if the render graph is dirty and needs recompilation.
		[[nodiscard]]
		bool IsDirty() const noexcept
		{
			return m_state == ERenderGraphState::Dirty;
		}

		/// Check if the render graph has been compiled and is ready for execution.
		[[nodiscard]]
		bool IsCompiled() const noexcept
		{
			return m_state == ERenderGraphState::Compiled;
		}

		/// Update an imported (external) resource's data in-place.
		///
		/// This replaces the underlying resource instance (e.g. swapchain
		/// backbuffer pointer) for an already-imported resource **without**
		/// modifying graph topology, resource IDs, or compilation state.
		/// The graph remains in the Compiled state after this call.
		///
		/// Use this inside a per-frame update callback to swap per-frame
		/// imported handles instead of tearing down and rebuilding the
		/// entire graph every frame.
		///
		/// @tparam T Must satisfy ResourceConcept and match the type used
		///           at the original Import() call site.
		/// @param id            The ResourceId returned by the original Import().
		/// @param newResource   The replacement resource instance to move in.
		/// @param incomingState State established by the caller. The caller must
		/// guarantee external readiness and imported RHI object lifetime. Even
		/// an unchanged pointer starts a new instance; old storage is retired separately.
		template <ResourceConcept T>
		[[nodiscard]]
		Hush::Result<void, EGraphError> UpdateImport(ResourceId id, T &&newResource,
													 Graphics::EResourceState incomingState)
		{
			if (!m_resourceManager.UpdateResource<T>(id, std::forward<T>(newResource)))
			{
				return EGraphError::InvalidResource;
			}
			m_importedResourceInitialStates[id] = incomingState;
			return Hush::Success();
		}

		/// Mark a compiled graph as dirty, forcing a full rebuild next frame.
		///
		/// Unlike Reset(), this does NOT clear passes, resources, or compilation
		/// data.  It simply flips the state flag so that the next frame's
		/// OnPreRender() takes the slow path (full Reset + rebuild + Compile).
		///
		/// Call this when something external invalidates the current graph
		/// structure — e.g. a window resize that changes render target
		/// dimensions, a scene change that adds/removes passes, etc.
		void Invalidate() noexcept
		{
			if (m_state == ERenderGraphState::Compiled)
			{
				ClearCompiledData();
			}
			m_state = ERenderGraphState::Dirty;
		}

		/// Reset the graph to allow rebuilding.
		///
		/// Clears all passes, resources, and compilation state. After reset,
		/// the graph is empty and dirty — ready for fresh pass registration.
		void Reset();

		[[nodiscard]]
		const std::vector<RenderPassNode> &GetPasses() const noexcept
		{
			return m_passes;
		}

		[[nodiscard]]
		std::vector<RenderPassNode> &GetPasses() noexcept
		{
			return m_passes;
		}

		[[nodiscard]]
		const std::vector<DependencyLevel> &GetDependencyLevels() const noexcept
		{
			return m_dependencyLevels;
		}

		[[nodiscard]]
		std::vector<DependencyLevel> &GetDependencyLevels() noexcept
		{
			return m_dependencyLevels;
		}

		/// Diagnostic count of compiled RAW/WAR/WAW edges, before synchronization culling.
		[[nodiscard]]
		size_t GetDependencyEdgeCount() const noexcept
		{
			size_t count = 0;
			for (const auto &edges : m_adjacencyList)
			{
				count += edges.size();
			}
			return count;
		}

		[[nodiscard]]
		uint32_t GetDetectedQueueCount() const noexcept
		{
			return m_detectedQueueCount;
		}

		[[nodiscard]]
		ResourceManager &GetResourceManager() noexcept
		{
			return m_resourceManager;
		}

		[[nodiscard]]
		const ResourceManager &GetResourceManager() const noexcept
		{
			return m_resourceManager;
		}

		/// Get the initial states declared for imported resources.
		/// The executor uses this to initialize its resource state tracker.
		[[nodiscard]]
		const boost::unordered_flat_map<ResourceId, Hush::Graphics::EResourceState> &GetImportedResourceInitialStates()
			const noexcept
		{
			return m_importedResourceInitialStates;
		}

	private:
		/// Builds the adjacency list representing pass dependencies.
		void BuildAdjacencyList(std::pmr::memory_resource &scratch);

		/// Rebuild all compiled state; no pointer survives a pass-vector mutation.
		void ClearCompiledData() noexcept;

		/// Iterative topological sort with release-build cycle detection.
		bool TopologicalSort(std::pmr::memory_resource &scratch);

		/// Build dependency levels using longest path algorithm
		void BuildDependencyLevels();

		/// Finalize dependency levels by organizing passes per queue
		void FinalizeDependencyLevels(std::pmr::memory_resource &scratch);

		/// Cull redundant synchronization points using SSIS algorithm
		void CullRedundantSyncPoints();
		void CullNodeSyncPoints(RenderPassNode &node, const RenderPassNode *previous) const;

	private:
		/// List of passes in the render graph
		std::vector<RenderPassNode> m_passes;

		/// Adjacency list representing pass dependencies
		std::vector<std::vector<uint32_t>> m_adjacencyList;

		/// Topologically sorted passes
		std::vector<RenderPassNode *> m_topologicalOrderedNodes;

		/// Dependency levels for parallel execution
		std::vector<DependencyLevel> m_dependencyLevels;

		ResourceManager m_resourceManager;
		// Keep the identity alive after executor destruction; a reused address is not the same context.
		std::shared_ptr<const void> m_executionContext;

		/// Initial resource states for imported (external) resources.
		/// Stored during build phase so the executor can initialize its
		/// resource state tracker without needing to be present at build time.
		boost::unordered_flat_map<ResourceId, Hush::Graphics::EResourceState> m_importedResourceInitialStates;

		ERenderGraphState m_state = ERenderGraphState::Dirty;
		EGraphError m_buildError = EGraphError::None;
		uint32_t m_detectedQueueCount = 1;
		uint32_t m_nextResourceId = 1;

		/// Lookup table mapping EPassType ordinal to physical queue index.
		/// Default is the identity mapping (Graphics=0, Compute=1, Transfer=2).
		/// Single-queue backends set all entries to 0.
		std::array<uint32_t, PASS_TYPE_COUNT> m_queueMap = {0, 1, 2};
	};
} // namespace Hush::RenderGraph
