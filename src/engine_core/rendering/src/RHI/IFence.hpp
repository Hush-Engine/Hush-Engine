/*! \file IFence.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Fence interface for cross-queue GPU synchronization
*/
#pragma once

#include <cstdint>
#include <limits>

namespace Hush::Graphics
{
	/// @brief Abstract fence interface for GPU synchronization.
	///
	/// A fence is a synchronization primitive used to coordinate work between
	/// the CPU and GPU, or between different GPU queues. This interface models
	/// a **timeline fence** (also known as a monotonic fence), where the fence
	/// holds a monotonically increasing uint64 value.
	///
	/// Timeline fences are the foundation of the render graph's cross-queue
	/// synchronization scheme described by the SSIS (Sufficient Synchronization
	/// Index Set) algorithm:
	///
	///   - A queue **signals** the fence to a specific value after completing work.
	///   - Another queue **waits** on the fence until it reaches a required value.
	///
	/// Each queue in the render graph owns one timeline fence. When the graph's
	/// execution encounters a cross-queue dependency it inserts a Signal on the
	/// producing queue and a Wait on the consuming queue for the same fence value.
	///
	/// @note On APIs that only support binary fences/semaphores (e.g. WebGPU),
	///       the backend implementation may emulate timeline behavior by chaining
	///       binary primitives or falling back to queue-level waits.
	///
	/// ### Typical usage inside the render graph executor
	/// ```
	/// // Producer queue finishes work, signals fence to value N
	/// producerQueue->Submit(cmdLists, /*signalFence=*/fence, /*signalValue=*/N);
	///
	/// // Consumer queue waits for fence to reach N before starting
	/// consumerQueue->Submit(cmdLists, /*waitFence=*/fence, /*waitValue=*/N);
	/// ```
	class IFence
	{
	public:
		/// Sentinel value representing an invalid / uninitialized fence value.
		static constexpr uint64_t INVALID_FENCE_VALUE = std::numeric_limits<uint64_t>::max();

		IFence() = default;
		virtual ~IFence() = default;

		IFence(const IFence &) = delete;
		IFence &operator=(const IFence &) = delete;
		IFence(IFence &&) = delete;
		IFence &operator=(IFence &&) = delete;

		/// @brief Get the last value that was signaled (completed) by the GPU.
		///
		/// This is a non-blocking query. The returned value may lag behind the
		/// most recently *requested* signal if the GPU has not caught up yet.
		///
		/// @return The most recent completed fence value.
		[[nodiscard]]
		virtual uint64_t GetCompletedValue() const = 0;

		/// @brief Get the last value that was requested to be signaled.
		///
		/// This tracks the highest value passed to Signal() or to a queue
		/// submission that references this fence, regardless of whether the
		/// GPU has completed it yet.
		///
		/// @return The most recent pending (or completed) signal value.
		[[nodiscard]]
		virtual uint64_t GetPendingValue() const = 0;

		/// @brief Block the calling CPU thread until the fence reaches the
		///        specified value or the timeout expires.
		///
		/// @param value    The fence value to wait for.
		/// @param timeoutNs  Maximum time to wait in nanoseconds.
		///                   Use UINT64_MAX for an infinite wait.
		/// @return true if the fence reached the requested value before the
		///         timeout, false on timeout.
		virtual bool WaitCPU(uint64_t value, uint64_t timeoutNs = std::numeric_limits<uint64_t>::max()) = 0;

		/// @brief Signal the fence to a specific value from the CPU.
		///
		/// This is rarely needed in normal render-graph operation (GPU-side
		/// signals via queue submission are preferred), but useful for:
		///   - Initial fence setup
		///   - Forcing completion from the CPU (e.g. during shutdown)
		///
		/// @param value The value to signal. Must be greater than the current
		///              completed value (timeline fences are monotonic).
		virtual void SignalCPU(uint64_t value) = 0;

		/// @brief Get the native API handle for this fence.
		///
		/// - **D3D12**: `ID3D12Fence*`
		/// - **Vulkan**: `VkSemaphore` (timeline semaphore)
		/// - **Metal**: `id<MTLSharedEvent>`
		/// - **WebGPU**: implementation-defined or nullptr
		///
		/// @return Opaque pointer to the native fence object.
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};

} // namespace Hush::Graphics
