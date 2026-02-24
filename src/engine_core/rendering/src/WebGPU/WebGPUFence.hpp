/*! \file WebGPUFence.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU implementation of IFence interface (CPU-emulated timeline fence)
*/
#pragma once

#include "../RHI/IFence.hpp"
#include <atomic>
#include <condition_variable>
#include <mutex>

namespace Hush::Graphics
{
	/// @brief WebGPU fence implementation — CPU-emulated timeline fence.
	///
	/// WebGPU does not expose native timeline fences (timeline semaphores).
	/// Since WebGPU only has a single queue, cross-queue synchronization is
	/// inherently serialized. This implementation emulates timeline fence
	/// semantics entirely on the CPU using an atomic counter and a condition
	/// variable for WaitCPU blocking.
	///
	/// This is sufficient for the render graph executor to function correctly:
	///   - Signal/Wait calls from SubmitBatched update the CPU-side counter.
	///   - Because WebGPU serializes all GPU work on one queue, ordering is
	///     already guaranteed by submission order.
	///   - WaitCPU can be used during shutdown or synchronization points.
	class WebGPUFence : public IFence
	{
	public:
		/// @brief Construct a CPU-emulated fence with the given initial value.
		/// @param initialValue Starting value for the fence counter (typically 0).
		explicit WebGPUFence(uint64_t initialValue = 0)
			: m_completedValue(initialValue),
			  m_pendingValue(initialValue)
		{
		}

		~WebGPUFence() override = default;

		WebGPUFence(const WebGPUFence &) = delete;
		WebGPUFence &operator=(const WebGPUFence &) = delete;
		WebGPUFence(WebGPUFence &&) = delete;
		WebGPUFence &operator=(WebGPUFence &&) = delete;

		/// @brief Get the last completed (signaled) value.
		///
		/// Since this is CPU-emulated, the completed value is updated
		/// immediately when SignalCPU or the queue's Signal method is called.
		[[nodiscard]]
		uint64_t GetCompletedValue() const override
		{
			return m_completedValue.load(std::memory_order_acquire);
		}

		/// @brief Get the last pending (requested) signal value.
		[[nodiscard]]
		uint64_t GetPendingValue() const override
		{
			return m_pendingValue.load(std::memory_order_acquire);
		}

		/// @brief Block the calling thread until the fence reaches the specified
		///        value or the timeout expires.
		///
		/// @param value     The fence value to wait for.
		/// @param timeoutNs Maximum wait time in nanoseconds (UINT64_MAX = infinite).
		/// @return true if the value was reached, false on timeout.
		bool WaitCPU(uint64_t value, uint64_t timeoutNs = std::numeric_limits<uint64_t>::max()) override
		{
			// Fast path: already reached
			if (m_completedValue.load(std::memory_order_acquire) >= value)
			{
				return true;
			}

			std::unique_lock<std::mutex> lock(m_mutex);

			if (timeoutNs == std::numeric_limits<uint64_t>::max())
			{
				m_cv.wait(lock, [&] { return m_completedValue.load(std::memory_order_acquire) >= value; });
				return true;
			}

			auto duration = std::chrono::nanoseconds(timeoutNs);
			return m_cv.wait_for(lock, duration,
								 [&] { return m_completedValue.load(std::memory_order_acquire) >= value; });
		}

		/// @brief Signal the fence to a specific value from the CPU.
		///
		/// Updates both the completed and pending values and wakes any threads
		/// blocked in WaitCPU.
		///
		/// @param value The value to signal (must be >= current completed value).
		void SignalCPU(uint64_t value) override
		{
			{
				std::lock_guard<std::mutex> lock(m_mutex);

				// Update pending value if this is higher
				uint64_t currentPending = m_pendingValue.load(std::memory_order_relaxed);
				if (value > currentPending)
				{
					m_pendingValue.store(value, std::memory_order_release);
				}

				// Update completed value
				m_completedValue.store(value, std::memory_order_release);
			}

			// Wake all waiters so they can re-check
			m_cv.notify_all();
		}

		/// @brief Mark a value as pending (requested but not yet completed).
		///
		/// This is used internally by the queue to track the highest requested
		/// signal value before the GPU has actually completed the work.
		///
		/// @param value The pending signal value.
		void SetPendingValue(uint64_t value)
		{
			uint64_t currentPending = m_pendingValue.load(std::memory_order_relaxed);
			if (value > currentPending)
			{
				m_pendingValue.store(value, std::memory_order_release);
			}
		}

		/// @brief Returns nullptr — WebGPU has no native fence object.
		[[nodiscard]]
		void *GetNativeHandle() const override
		{
			return nullptr;
		}

	private:
		/// The last value that has been "completed" (signaled).
		std::atomic<uint64_t> m_completedValue;

		/// The last value that has been requested to be signaled.
		std::atomic<uint64_t> m_pendingValue;

		/// Mutex protecting the condition variable.
		mutable std::mutex m_mutex;

		/// Condition variable for WaitCPU blocking.
		std::condition_variable m_cv;
	};

} // namespace Hush::Graphics
