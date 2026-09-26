/*! \file RenderGraphExecutor.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-18
	\brief Device-specific access, transition and submission planning.
*/
#pragma once

#include "RenderGraph.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include <array>
#include <deque>
#include <vector>

namespace Hush::RenderGraph
{
	/// Plans all operations before recording or submitting any work. A dependency
	/// level is a scheduling aid, never an implicit all-queue GPU barrier.
	/// Native resources must use shared/concurrent queue-family ownership; this
	/// RHI does not yet express exclusive-family release/acquire transfers.
	class RenderGraphExecutor
	{
	public:
		explicit RenderGraphExecutor(Graphics::IGraphicsDevice *device);
		~RenderGraphExecutor();
		RenderGraphExecutor(const RenderGraphExecutor &) = delete;
		RenderGraphExecutor &operator=(const RenderGraphExecutor &) = delete;
		RenderGraphExecutor(RenderGraphExecutor &&) = delete;
		RenderGraphExecutor &operator=(RenderGraphExecutor &&) = delete;

		/// A graph remains bound to this executor until Graph::Reset; state histories
		/// and timelines cannot be silently transferred to another executor.
		/// Scratch storage is call-scoped and may be reset after Execute returns,
		/// even while GPU work is pending. Persistent state, command lists, payloads
		/// and retirement records remain independently owned. The caller owns reset.
		[[nodiscard]]
		Hush::Result<void, EGraphError> Execute(RenderGraph &graph,
												std::pmr::memory_resource &scratch = *std::pmr::get_default_resource());

		/// Clears scratch planning state. Live fence counters NEVER reset.
		void ResetFrameState();

		/// Nonblocking retirement. The device must outlive this executor and graph resources.
		void RetireCompleted();
		/// Exceptional rebuild/shutdown only; ordinary frames never drain queues.
		void WaitIdle();

		struct ResourceCompletion
		{
			Graphics::EResourceState state = Graphics::EResourceState::Undefined;
			std::array<Graphics::FenceWaitDescriptor, RenderGraph::PASS_TYPE_COUNT> points{};
		};
		/// Last scheduled state plus outstanding users. Non-null fences are owned by
		/// this executor: callers may wait/query, but must not signal them.
		/// Imported RHI object lifetime/readiness remains caller-owned.
		[[nodiscard]]
		Hush::Result<ResourceCompletion, EGraphError> GetResourceCompletion(const ResourceHandle &resource) const;

	private:
		using Points = std::array<uint64_t, RenderGraph::PASS_TYPE_COUNT>;
		static constexpr uint32_t NO_QUEUE = std::numeric_limits<uint32_t>::max();

		struct State
		{
			Graphics::EResourceState state = Graphics::EResourceState::Undefined;
			Points ready{};	   // Last writer or state-establishing operation.
			Points accesses{}; // Every outstanding reader/writer, per physical queue.
			bool lastWrite = false;
		};

		struct PersistentState
		{
			State scheduled;
			std::weak_ptr<const void> lifetime;
		};

		struct InFlight
		{
			Points completion{};
			std::vector<std::shared_ptr<const void>> lifetimes;
			std::vector<std::unique_ptr<Graphics::ICommandList>> commands;
		};

		struct Access
		{
			ResourceId id;
			Graphics::EResourceState state = Graphics::EResourceState::Undefined;
			bool write = false;
			bool physical = false;
			uint32_t barrierQueue = NO_QUEUE;
		};

		struct PassAccesses
		{
			RenderPassNode *node = nullptr;
			std::vector<Access> accesses;
		};

		struct Operation
		{
			uint32_t queue = 0;
			RenderPassNode *pass = nullptr; // nullptr: dedicated barrier operation.
			Graphics::BarrierGroup barriers;
			Points waits{};
			uint64_t signal = 0;
		};

		Hush::Result<void, EGraphError> InitializeQueues();
		Hush::Result<void, EGraphError> Initialize(RenderGraph &graph, std::pmr::memory_resource &scratch);
		bool GatherPassAccesses(PassAccesses &pass, const ResourceManager &resources) const;
		void ApplyPassState(const PassAccesses &pass, const Operation &work, const Points &transitions);
		Hush::Result<void, EGraphError> GatherAccesses(RenderGraph &graph, std::pmr::memory_resource &scratch);
		void CombineReadEpochs(uint32_t resourceCount, std::pmr::memory_resource &scratch);
		Hush::Result<void, EGraphError> PlanPass(PassAccesses &pass, const ResourceManager &resources);
		Hush::Result<void, EGraphError> SubmitPlan(RenderGraph &graph, std::pmr::memory_resource &scratch);
		[[nodiscard]]
		uint32_t TransitionQueue(uint32_t preferred, Graphics::EResourceState before,
								 Graphics::EResourceState after) const;
		[[nodiscard]]
		std::unique_ptr<Graphics::ICommandList> CreateCommandList(uint32_t queue) const;
		static void Join(Points &destination, const Points &source);

		Graphics::IGraphicsDevice *m_device;
		std::shared_ptr<const void> m_executionContext = std::make_shared<char>();
		uint32_t m_queueCount = 0;
		std::array<Graphics::ICommandQueue *, RenderGraph::PASS_TYPE_COUNT> m_queues{};
		std::array<std::unique_ptr<Graphics::IFence>, RenderGraph::PASS_TYPE_COUNT> m_fences;
		Points m_fenceValues{};
		std::vector<State> m_states;
		boost::unordered_flat_map<uint64_t, PersistentState> m_persistentStates;
		std::deque<InFlight> m_inFlight;
		bool m_submissionFailed = false; // A partial/unknown submission is terminal for this executor.
		std::vector<PassAccesses> m_schedule;
		std::vector<Operation> m_plan;
	};
} // namespace Hush::RenderGraph
