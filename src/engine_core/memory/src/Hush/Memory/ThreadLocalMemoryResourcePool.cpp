/*! \file ThreadLocalMemoryResourcePool.cpp
	\author Alan Ramirez Herrera
	\date 2026-07-01
	\brief Implementation of ThreadLocalMemoryResourcePool.
*/

#include "Hush/Memory/ThreadLocalMemoryResourcePool.hpp"

#include <atomic>
#include <cstdint>
#include <unordered_map>

namespace Hush::Memory
{
	namespace
	{
		// Hands out a unique, never-reused id per pool instance (see m_generation).
		std::atomic<std::uint64_t> g_generationCounter{1};
	} // namespace

	ThreadLocalMemoryResourcePool::ThreadLocalMemoryResourcePool(std::size_t initialArenaSize,
																 std::pmr::memory_resource *upstream) noexcept
		: m_generation(g_generationCounter.fetch_add(1, std::memory_order_relaxed)),
		  m_initialArenaSize(initialArenaSize),
		  m_upstream(upstream),
		  m_countingUpstream(upstream)
	{
	}

	ThreadLocalMemoryResourcePool::~ThreadLocalMemoryResourcePool() = default;

	ThreadLocalMemoryResourcePool::Arena &ThreadLocalMemoryResourcePool::GetLocalArena()
	{
		// Fast path: a thread-local single-entry cache keyed by the pool's unique generation id.
		// Allocations cluster by pool (a batch of frame allocations, then a batch of scene
		// allocations), so this hits nearly always and avoids the hash lookup below on the hot
		// path. Keying by the never-reused generation (not `this`) avoids a false hit when a new
		// pool reuses a destroyed pool's address. cachedGeneration 0 never matches a real id.
		thread_local std::uint64_t cachedGeneration = 0;
		thread_local Arena *cachedArena = nullptr;
		if (cachedGeneration == m_generation)
		{
			return *cachedArena;
		}

		// Slow path: per-thread map keyed by generation, so the frame and scene pools can share
		// the same thread without colliding. The cached Arena* points into m_arenas, which owns it.
		thread_local std::unordered_map<std::uint64_t, Arena *> tlsArenas;

		Arena *arenaPtr = nullptr;
		if (const auto it = tlsArenas.find(m_generation); it != tlsArenas.end())
		{
			arenaPtr = it->second;
		}
		else
		{
			// First allocation on this thread for this pool: create the arena and register it so
			// Reset() (and teardown) can reach it. Arenas spill through m_countingUpstream so the
			// pool can measure exactly how much overflowed the initial buffers.
			auto arena = std::make_unique<Arena>(m_initialArenaSize, &m_countingUpstream);
			arenaPtr = arena.get();
			{
				std::lock_guard lock(m_mutex);
				m_arenas.push_back(std::move(arena));
			}
			tlsArenas.emplace(m_generation, arenaPtr);
		}

		cachedGeneration = m_generation;
		cachedArena = arenaPtr;
		return *arenaPtr;
	}

	void *ThreadLocalMemoryResourcePool::do_allocate(std::size_t bytes, std::size_t alignment)
	{
		Arena &arena = GetLocalArena();
		arena.bytesRequested += bytes;
		++arena.allocationCount;
		return arena.resource->allocate(bytes, alignment);
	}

	void ThreadLocalMemoryResourcePool::do_deallocate(void * /*ptr*/, std::size_t /*bytes*/,
													  std::size_t /*alignment*/) noexcept
	{
		// Monotonic: individual deallocations are no-ops. Memory is reclaimed en masse by Reset().
	}

	bool ThreadLocalMemoryResourcePool::do_is_equal(const std::pmr::memory_resource &other) const noexcept
	{
		return this == &other;
	}

	void ThreadLocalMemoryResourcePool::Reset()
	{
		std::lock_guard lock(m_mutex);

		// Snapshot spillover (upstream traffic since the last Reset), then zero it for the next
		// cycle. Safe to reset here: no thread is allocating at the barrier.
		m_lastSpilledBytes = m_countingUpstream.spilledBytes.exchange(0, std::memory_order_relaxed);
		m_lastSpilledAllocations = m_countingUpstream.spilledAllocations.exchange(0, std::memory_order_relaxed);

		std::size_t bytesRequested = 0;
		std::size_t allocationCount = 0;
		for (const std::unique_ptr<Arena> &arena : m_arenas)
		{
			bytesRequested += arena->bytesRequested;
			allocationCount += arena->allocationCount;
			arena->bytesRequested = 0;
			arena->allocationCount = 0;
			arena->Rewind();
		}

		m_lastBytesRequested = bytesRequested;
		m_lastAllocationCount = allocationCount;
		m_highWaterBytes = std::max(m_highWaterBytes, bytesRequested);
		m_arenaCount = m_arenas.size();
	}

	ThreadLocalMemoryResourcePool::Stats ThreadLocalMemoryResourcePool::GetStats() const noexcept
	{
		return Stats{
			.bytesRequestedLastCycle = m_lastBytesRequested,
			.allocationCountLastCycle = m_lastAllocationCount,
			.spilledBytesLastCycle = m_lastSpilledBytes,
			.spilledAllocationsLastCycle = m_lastSpilledAllocations,
			.highWaterBytes = m_highWaterBytes,
			.arenaCount = m_arenaCount,
			.initialArenaSize = m_initialArenaSize,
		};
	}
} // namespace Hush::Memory
