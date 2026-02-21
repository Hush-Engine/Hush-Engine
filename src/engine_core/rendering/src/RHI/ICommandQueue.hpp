/*! \file ICommandQueue.hpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief Command queue interface for graphics abstraction.
		   Based on Pavlov's "Organizing GPU Work with Directed Acyclic Graphs" article.
*/
#pragma once

#include "GraphicsTypes.hpp"
#include "ICommandList.hpp"
#include "IFence.hpp"
#include <span>
#include <vector>

namespace Hush::Graphics
{
	/// @brief Describes a single fence wait operation.
	///
	/// The queue will stall until the fence reaches the specified value
	/// before executing any command lists in the same SubmitInfo batch.
	struct FenceWaitDescriptor
	{
		IFence *fence = nullptr;
		uint64_t value = 0;
	};

	/// @brief Describes a single fence signal operation.
	///
	/// After all command lists in the same SubmitInfo batch have been
	/// submitted, the queue will signal the fence to the specified value.
	struct FenceSignalDescriptor
	{
		IFence *fence = nullptr;
		uint64_t value = 0;
	};

	/// @brief Describes a batched submission to a command queue.
	struct SubmitInfo
	{
		/// Fences to wait on before executing command lists.
		/// The queue will not begin executing until all wait conditions are met.
		std::vector<FenceWaitDescriptor> waitFences;

		/// Command lists to execute, in order.
		std::vector<ICommandList *> commandLists;

		/// Fences to signal after all command lists have completed execution.
		std::vector<FenceSignalDescriptor> signalFences;
	};

	/// @brief Abstract command queue for command submission
	class ICommandQueue
	{
	public:
		ICommandQueue() = default;
		virtual ~ICommandQueue() = default;

		ICommandQueue(const ICommandQueue &) = delete;
		ICommandQueue &operator=(const ICommandQueue &) = delete;
		ICommandQueue(ICommandQueue &&) = delete;
		ICommandQueue &operator=(ICommandQueue &&) = delete;

		/// @brief Get queue type
		[[nodiscard]]
		virtual EQueueType GetQueueType() const = 0;

		/// @brief Submit command lists for execution (simple, no synchronization).
		///
		/// This is the simplest form of submission — no fence waits or signals
		/// are associated with the call. Useful for single-queue scenarios or
		/// when synchronization is managed externally.
		///
		/// @param commandLists Array of command lists to submit, executed in order.
		virtual void Submit(std::span<ICommandList *> commandLists) = 0;

		/// @brief Submit a batch of command lists with explicit fence synchronization.
		///
		/// This is the primary submission method used by the render graph executor.
		/// It maps to the article's concept of a **command list batch**: a group of
		/// command lists that share the same set of fence waits (before) and fence
		/// signals (after).
		///
		/// The implementation must:
		///   1. Wait on all fences in submitInfo.waitFences before beginning execution.
		///   2. Execute all command lists in submitInfo.commandLists, in order.
		///   3. Signal all fences in submitInfo.signalFences after execution completes.
		///
		/// @param submitInfo Batched submission descriptor containing waits,
		///                   command lists, and signals.
		virtual void SubmitBatched(const SubmitInfo &submitInfo) = 0;

		/// @brief Signal a fence from this queue without submitting any command lists.
		///
		/// Enqueues a GPU-side signal for the given fence to the specified value.
		/// The signal will occur after all previously submitted work on this queue
		/// has completed.
		///
		/// @param fence  The fence to signal.
		/// @param value  The value to signal the fence to (must be monotonically
		///               increasing for timeline fences).
		virtual void Signal(IFence *fence, uint64_t value) = 0;

		/// @brief Enqueue a GPU-side wait on a fence before any subsequent work.
		///
		/// All command lists submitted after this call will not begin execution
		/// until the fence reaches the specified value. This is a GPU-side wait
		/// only — it does not block the CPU.
		///
		/// @param fence  The fence to wait on.
		/// @param value  The value the fence must reach before the wait is satisfied.
		virtual void Wait(IFence *fence, uint64_t value) = 0;

		/// @brief Wait for all commands on this queue to complete (CPU-side stall).
		///
		/// Blocks the calling thread until the GPU has finished executing all
		/// previously submitted command lists on this queue.
		virtual void WaitIdle() = 0;

		/// @brief Get native handle (API-specific)
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};
} // namespace Hush::Graphics
