/*! \file RenderGraphExecutor.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-18
	\brief Explicit resource-access and completion-point planning.
*/
#include "RenderGraphExecutor.hpp"
#include <algorithm>
#include <bit>

using namespace Hush::RenderGraph;
using namespace Hush::Graphics;

namespace
{
	bool ValidAccessState(EResourceState state, bool write)
	{
		const auto bits = static_cast<uint32_t>(state);
		if (write)
		{
			return IsWriteState(state) && std::has_single_bit(bits);
		}
		constexpr auto readMask = static_cast<uint32_t>(EResourceState::GenericRead) |
								  static_cast<uint32_t>(EResourceState::DepthStencilRead) |
								  static_cast<uint32_t>(EResourceState::Present);
		return state == EResourceState::UnorderedAccess || (bits != 0 && (bits & ~readMask) == 0);
	}
} // namespace

RenderGraphExecutor::RenderGraphExecutor(IGraphicsDevice *device)
	: m_device(device)
{
}

RenderGraphExecutor::~RenderGraphExecutor()
{
	if (!m_inFlight.empty())
	{
		WaitIdle();
	}
}

void RenderGraphExecutor::RetireCompleted()
{
	if (m_device != nullptr)
	{
		m_device->PollCompletions();
	}
	std::erase_if(m_inFlight, [&](const InFlight &work) {
		for (uint32_t q = 0; q < m_queueCount; ++q)
		{
			if (work.completion[q] != 0 && m_fences[q]->GetCompletedValue() < work.completion[q])
			{
				return false;
			}
		}
		return true;
	});
	for (auto it = m_persistentStates.begin(); it != m_persistentStates.end();)
	{
		if (it->second.lifetime.expired())
		{
			m_persistentStates.erase(it++);
		}
		else
		{
			++it;
		}
	}
}

void RenderGraphExecutor::WaitIdle()
{
	auto queues = m_queues;
	// External uploads may precede the first graph execution.
	if (m_queueCount == 0 && m_device != nullptr)
	{
		for (uint32_t q = 0; q < queues.size(); ++q)
		{
			queues[q] = m_device->GetQueueForType(static_cast<EQueueType>(q));
		}
	}
	for (size_t q = 0; q < queues.size(); ++q)
	{
		if (queues[q] != nullptr && std::find(queues.begin(), queues.begin() + q, queues[q]) == queues.begin() + q)
		{
			queues[q]->WaitIdle();
		}
	}
	m_inFlight.clear();
}

Hush::Result<RenderGraphExecutor::ResourceCompletion, EGraphError> RenderGraphExecutor::GetResourceCompletion(
	const ResourceHandle &resource) const
{
	const auto it = m_persistentStates.find(resource.GetInstanceId());
	if (it == m_persistentStates.end() || m_submissionFailed)
	{
		return EGraphError::InvalidResource;
	}
	const auto &state = it->second.scheduled;
	ResourceCompletion result{.state = state.state, .points = {}};
	for (uint32_t q = 0; q < m_queueCount; ++q)
	{
		const auto value = std::max(state.ready[q], state.accesses[q]);
		if (value != 0)
		{
			result.points[q] = {.fence = m_fences[q].get(), .value = value};
		}
	}
	return result;
}

void RenderGraphExecutor::ResetFrameState()
{
	RetireCompleted();
	if (m_states.size() <= 1)
	{
		m_schedule.clear(); // No resource accesses: there are no inner allocations to preserve.
	}
	else
	{
		for (auto &pass : m_schedule)
		{
			pass.node = nullptr;
			pass.accesses.clear(); // Retain capacity, never references into a previous graph.
		}
	}
	m_states.clear();
	m_plan.clear();
	// Signal values belong to the fence lifetime, not to a CPU frame.
}

void RenderGraphExecutor::Join(Points &destination, const Points &source)
{
	for (size_t q = 0; q < destination.size(); ++q)
	{
		destination[q] = std::max(destination[q], source[q]);
	}
}

Hush::Result<void, EGraphError> RenderGraphExecutor::InitializeQueues()
{
	if (m_device == nullptr)
	{
		return EGraphError::DeviceFailure;
	}
	uint32_t queueCount = 0;
	std::array<Hush::Graphics::ICommandQueue *, RenderGraph::PASS_TYPE_COUNT> queues{};
	for (uint32_t type = 0; type < RenderGraph::PASS_TYPE_COUNT; ++type)
	{
		const auto logical = static_cast<EQueueType>(type);
		const auto physical = m_device->MapPassTypeToQueueIndex(logical);
		if (physical >= RenderGraph::PASS_TYPE_COUNT)
		{
			return EGraphError::InvalidQueue;
		}
		auto *queue = m_device->GetQueueForType(logical);
		if (queue == nullptr || queue != m_device->GetQueueForType(static_cast<EQueueType>(physical)))
		{
			return EGraphError::InvalidQueue;
		}
		queues[physical] = queue;
		queueCount = std::max(queueCount, physical + 1);
	}
	// A live timeline and its retirement records belong to one physical queue.
	if (m_queueCount != 0 && queues != m_queues)
	{
		return EGraphError::InvalidQueue;
	}
	for (uint32_t q = 0; q < queueCount; ++q)
	{
		if (queues[q] == nullptr)
		{
			continue;
		}
		for (uint32_t prior = 0; prior < q; ++prior)
		{
			if (queues[prior] == queues[q])
			{
				return EGraphError::InvalidQueue;
			}
		}
		if (!m_fences[q])
		{
			m_fences[q] = m_device->CreateFence(0);
		}
		if (!m_fences[q])
		{
			return EGraphError::DeviceFailure;
		}
	}

	m_queues = queues;
	m_queueCount = queueCount;
	return Hush::Success();
}

Hush::Result<void, EGraphError> RenderGraphExecutor::Initialize(RenderGraph &graph, std::pmr::memory_resource &scratch)
{
	if (graph.m_executionContext && graph.m_executionContext != m_executionContext)
	{
		return EGraphError::InvalidState;
	}
	const auto queues = InitializeQueues();
	if (!queues.has_value())
	{
		return queues.error();
	}
	graph.m_executionContext = m_executionContext;
	m_states.assign(graph.m_nextResourceId, State{});
	boost::unordered_flat_set<void *, boost::hash<void *>, std::equal_to<>, std::pmr::polymorphic_allocator<void *>>
		identities{std::pmr::polymorphic_allocator<void *>{&scratch}};
	EGraphError error = EGraphError::None;
	graph.GetResourceManager().ForEachResource([&](ResourceId id, ResourceHandle &handle) {
		handle.CreateResource(m_device);
		if (handle.SupportsBarriers())
		{
			auto *resource = handle.GetBarrierResource();
			if (resource == nullptr)
			{
				error = EGraphError::InvalidResource;
			}
			else if (!identities.insert(resource).second)
			{
				error = EGraphError::DuplicateResource;
			}
		}
		State initialState;
		const auto &initial = graph.GetImportedResourceInitialStates();
		if (auto it = initial.find(id); it != initial.end())
		{
			initialState.state = it->second;
		}
		// Allocate entries before submission. Planning uses a scratch copy and
		// cannot corrupt the last successfully scheduled state on failure.
		auto [entry, inserted] = m_persistentStates.try_emplace(
			handle.GetInstanceId(), PersistentState{.scheduled = initialState, .lifetime = handle.GetLifetimeToken()});
		m_states[id.id] = entry->second.scheduled;
	});
	if (error != EGraphError::None)
	{
		return error;
	}
	return Hush::Success();
}

bool RenderGraphExecutor::GatherPassAccesses(PassAccesses &pass, const ResourceManager &resources) const
{
	const auto *node = pass.node;
	auto addAccess = [&](ResourceId id, EResourceState state, bool write) {
		const auto *handle = resources.GetResourceHandle(id);
		if (handle == nullptr)
		{
			return false;
		}
		const bool physical = handle->SupportsBarriers();
		if (physical && !ValidAccessState(state, write))
		{
			return false;
		}
		if (physical && write && node->GetReadResources().contains(id) && node->GetReadState(id) != state)
		{
			return false; // Internal state changes need explicit callback barriers or separate passes.
		}
		pass.accesses.push_back(
			{.id = id, .state = physical ? state : EResourceState::Undefined, .write = write, .physical = physical});
		return true;
	};
	for (auto id : node->GetWrittenResources())
	{
		if (!addAccess(id, node->GetWriteState(id), true))
		{
			return false;
		}
	}
	return std::ranges::all_of(node->GetReadResources(), [&](ResourceId id) {
		return node->GetWrittenResources().contains(id) || addAccess(id, node->GetReadState(id), false);
	});
}

Hush::Result<void, EGraphError> RenderGraphExecutor::GatherAccesses(RenderGraph &graph,
																	std::pmr::memory_resource &scratch)
{
	const bool reuseAccesses = graph.GetResourceManager().GetResourceCount() != 0;
	if (reuseAccesses)
	{
		m_schedule.resize(graph.GetPasses().size());
	}
	else
	{
		m_schedule.clear();
		m_schedule.reserve(graph.GetPasses().size());
	}
	size_t index = 0;
	for (const auto &level : graph.GetDependencyLevels())
	{
		for (auto *node : level.GetPassNodes())
		{
			if (node->GetQueueIndex() != m_device->MapPassTypeToQueueIndex(PassTypeToQueueType(node->GetPassType())))
			{
				return EGraphError::InvalidQueue;
			}
			auto &pass = reuseAccesses ? m_schedule[index++] : m_schedule.emplace_back();
			pass.node = node;
			pass.accesses.clear();
			if (!GatherPassAccesses(pass, graph.GetResourceManager()))
			{
				return EGraphError::InvalidState;
			}
		}
	}
	CombineReadEpochs(graph.m_nextResourceId, scratch);
	return Hush::Success();
}

void RenderGraphExecutor::CombineReadEpochs(uint32_t resourceCount, std::pmr::memory_resource &scratch)
{
	// All vectors have their final size before taking access pointers. Readers
	// may span dependency levels: a CPU level boundary is not synchronization.
	std::pmr::vector<std::pmr::vector<Access *>> histories(resourceCount, &scratch);
	for (auto &pass : m_schedule)
	{
		for (auto &access : pass.accesses)
		{
			histories[access.id.id].push_back(&access);
		}
	}
	for (const auto &history : histories)
	{
		for (size_t first = 0; first < history.size();)
		{
			if (history[first]->write)
			{
				++first;
				continue;
			}
			auto combined = history[first]->state;
			size_t end = first + 1;
			while (end < history.size() && !history[end]->write &&
				   m_device->CanCombineReadStates(combined, history[end]->state))
			{
				combined |= history[end++]->state;
			}
			for (; first < end; ++first)
			{
				history[first]->state = combined;
			}
		}
	}
}

uint32_t RenderGraphExecutor::TransitionQueue(uint32_t preferred, EResourceState before, EResourceState after) const
{
	const auto required = static_cast<uint32_t>(before | after);
	auto supports = [&](uint32_t q) {
		return m_queues[q] != nullptr &&
			   (required & ~m_device->GetQueueSupportedStates(static_cast<EQueueType>(q))) == 0;
	};
	if (supports(preferred))
	{
		return preferred;
	}
	for (uint32_t q = 0; q < m_queueCount; ++q)
	{
		if (supports(q))
		{
			return q;
		}
	}
	return NO_QUEUE;
}

Hush::Result<void, EGraphError> RenderGraphExecutor::PlanPass(PassAccesses &pass, const ResourceManager &resources)
{
	Operation work;
	work.queue = pass.node->GetQueueIndex();
	work.pass = pass.node;
	for (const auto *dependency : pass.node->GetNodesToSync())
	{
		auto &wait = work.waits[dependency->GetQueueIndex()];
		wait = std::max(wait, dependency->m_fenceSignalValue);
	}
	std::array<Operation, RenderGraph::PASS_TYPE_COUNT> barriers;
	for (auto &access : pass.accesses)
	{
		auto &history = m_states[access.id.id];
		if (!access.write && m_device->CanCombineReadStates(history.state, access.state) &&
			(history.state & access.state) == access.state)
		{
			access.state = history.state; // Keep a valid read superset; do not narrow it.
		}
		const bool transition = access.physical && history.state != access.state;
		const bool used =
			std::any_of(history.accesses.begin(), history.accesses.end(), [](auto value) { return value != 0; });
		const bool uav = access.physical && !transition && access.state == EResourceState::UnorderedAccess && used &&
						 (access.write || history.lastWrite);
		if (!transition && !uav)
		{
			Join(work.waits, history.ready);
			if (access.write)
			{
				Join(work.waits, history.accesses);
			}
			continue;
		}
		const auto queue = TransitionQueue(work.queue, history.state, access.state);
		if (queue == NO_QUEUE)
		{
			return EGraphError::UnsupportedTransition;
		}
		access.barrierQueue = queue;
		auto &operation = barriers[queue];
		operation.queue = queue;
		Join(operation.waits, history.ready);
		Join(operation.waits, history.accesses);
		auto *resource = resources.GetResourceHandle(access.id)->GetBarrierResource();
		if (transition)
		{
			operation.barriers.transitions.push_back(
				{.resource = resource, .stateBefore = history.state, .stateAfter = access.state});
		}
		else
		{
			operation.barriers.uavBarriers.push_back({resource});
		}
	}
	// Capture every prerequisite before allocating any transition signal. Each
	// consumer uses this operation's exact signal, never a mutable latest value.
	Points transitionSignals{};
	for (auto &operation : barriers)
	{
		if (operation.barriers.transitions.empty() && operation.barriers.uavBarriers.empty())
		{
			continue;
		}
		operation.signal = ++m_fenceValues[operation.queue];
		transitionSignals[operation.queue] = operation.signal;
		work.waits[operation.queue] = std::max(work.waits[operation.queue], operation.signal);
		m_plan.push_back(std::move(operation));
	}
	work.signal = ++m_fenceValues[work.queue];
	pass.node->m_fenceSignalValue = work.signal;
	ApplyPassState(pass, work, transitionSignals);
	m_plan.push_back(std::move(work));
	return Hush::Success();
}

void RenderGraphExecutor::ApplyPassState(const PassAccesses &pass, const Operation &work, const Points &transitions)
{
	for (const auto &access : pass.accesses)
	{
		auto &history = m_states[access.id.id];
		if (access.barrierQueue != NO_QUEUE)
		{
			history.state = access.state;
			history.ready = {};
			history.accesses = {};
			history.ready[access.barrierQueue] = transitions[access.barrierQueue];
		}
		if (access.write)
		{
			history.ready = {};
			history.accesses = {};
			history.ready[work.queue] = work.signal;
		}
		history.accesses[work.queue] = work.signal;
		history.lastWrite = access.write;
	}
}

std::unique_ptr<ICommandList> RenderGraphExecutor::CreateCommandList(uint32_t queue) const
{
	switch (queue)
	{
	case 1:
		return m_device->CreateComputeCommandList();
	case 2:
		return m_device->CreateCopyCommandList();
	default:
		return m_device->CreateGraphicsCommandList();
	}
}

Hush::Result<void, EGraphError> RenderGraphExecutor::SubmitPlan(RenderGraph &graph, std::pmr::memory_resource &scratch)
{
	if (m_plan.empty())
	{
		return Hush::Success();
	}
	InFlight work;
	work.lifetimes.reserve(graph.GetResourceManager().GetResourceCount() + graph.GetPasses().size());
	graph.GetResourceManager().ForEachResource(
		[&](ResourceId, const ResourceHandle &handle) { work.lifetimes.push_back(handle.GetLifetimeToken()); });
	for (const auto &pass : graph.GetPasses())
	{
		work.lifetimes.push_back(pass.GetLifetimeToken());
	}
	auto &commands = work.commands;
	commands.reserve(m_plan.size());
	// Only the outer staging array is scratch. SubmitInfo's RHI-owned vector
	// members and in-flight command/resource ownership keep their normal allocators.
	std::pmr::vector<SubmitInfo> submissions(m_plan.size(), &scratch);
	for (auto &operation : m_plan)
	{
		auto cmd = CreateCommandList(operation.queue);
		if (!cmd)
		{
			return EGraphError::DeviceFailure;
		}
		cmd->Reset();
		if (!operation.barriers.transitions.empty())
		{
			cmd->ResourceBarrier(operation.barriers.transitions);
		}
		if (!operation.barriers.uavBarriers.empty())
		{
			cmd->UAVBarrier(operation.barriers.uavBarriers);
		}
		if (operation.pass != nullptr)
		{
			operation.pass->Execute(cmd.get(), graph.GetResourceManager());
		}
		cmd->Close();
		commands.push_back(std::move(cmd));
	}
	for (size_t i = 0; i < m_plan.size(); ++i)
	{
		const auto &operation = m_plan[i];
		auto &submit = submissions[i];
		for (uint32_t q = 0; q < m_queueCount; ++q)
		{
			if (q != operation.queue && operation.waits[q] != 0)
			{
				submit.waitFences.push_back({.fence = m_fences[q].get(), .value = operation.waits[q]});
			}
		}
		submit.commandLists.push_back(commands[i].get());
		submit.signalFences.push_back({.fence = m_fences[operation.queue].get(), .value = operation.signal});
		work.completion[operation.queue] = operation.signal;
	}
	// All allocation/recording completes before the first submit. Even a
	// partially failed submission retains every object until an explicit drain.
	m_inFlight.push_back(std::move(work));
	try
	{
		for (size_t i = 0; i < m_plan.size(); ++i)
		{
			m_queues[m_plan[i].queue]->SubmitBatched(submissions[i]);
		}
	}
	catch (...)
	{
		m_submissionFailed = true;
		return EGraphError::DeviceFailure;
	}
	graph.GetResourceManager().ForEachResource([&](ResourceId id, const ResourceHandle &handle) {
		m_persistentStates.at(handle.GetInstanceId()).scheduled = m_states[id.id];
	});
	return Hush::Success();
}

Hush::Result<void, EGraphError> RenderGraphExecutor::Execute(RenderGraph &graph, std::pmr::memory_resource &scratch)
{
	if (m_submissionFailed)
	{
		return EGraphError::DeviceFailure;
	}
	RetireCompleted();
	if (!graph.IsCompiled())
	{
		return EGraphError::NotCompiled;
	}
	m_plan.clear();
	auto initialized = Initialize(graph, scratch);
	if (!initialized.has_value())
	{
		return initialized.error();
	}
	auto gathered = GatherAccesses(graph, scratch);
	if (!gathered.has_value())
	{
		return gathered.error();
	}
	for (auto &pass : m_schedule)
	{
		auto result = PlanPass(pass, graph.GetResourceManager());
		if (!result.has_value())
		{
			return result.error();
		}
	}
	return SubmitPlan(graph, scratch);
}
