/*! \file StealingQueue.hpp
	\author Alan Ramirez
	\date 2025-07-03
	\brief Stealing queue implementation
*/

#pragma once
#include <cstddef>
#include <cstdint>
#include <atomic>
#include <new>
#include <array>
#include <limits>
#include <tuple>
#include <optional>
#include "Platform.hpp"
#include "Result.hpp"

namespace Hush::Threading
{
	/// Enumeration for errors that can occur during stealing operations.
	enum class EStealError : uint8_t
	{
		/// The queue is empty, and no items can be stolen.
		Empty,
		/// The queue is busy, and stealing cannot proceed at this time.
		Busy,
	};

	template <typename T, size_t Size>
	class Worker;

	template <typename T, size_t Size>
	class Stealer;

	/// Pack two 32-bit integers into a 64-bit integer.
	/// @param workerHead Worker's head index.
	/// @param stealerHead Stealer's head index.
	/// @return A 64-bit integer containing both indices packed together.
	constexpr uint64_t Pack(uint32_t workerHead, uint32_t stealerHead) noexcept
	{
		return (static_cast<uint64_t>(workerHead) << 32) | static_cast<uint64_t>(stealerHead);
	}

	/// Unpack a 64-bit integer into two 32-bit integers.
	/// @param value The 64-bit integer to unpack.
	/// @return A tuple containing the worker's head index and the stealer's head index.
	constexpr std::tuple<uint32_t, uint32_t> Unpack(uint64_t value) noexcept
	{
		return {static_cast<uint32_t>(value >> 32),
				static_cast<uint32_t>(value & std::numeric_limits<uint32_t>::max())};
	}

	/// This queue is heavily inspired by the work of asynchronics/st3, which is a Rust implementation of a
	/// stealing queue. Its work is MIT-licensed and can be found at: https://github.com/asynchronics/st3
	///
	/// Its work is based on Tokio's worker queue, itself based on Go scheduler work-stealing queues.
	///
	/// Unlike his implementation, we use a statically-sized array to store the data because it is meant to be used by
	/// our thread pool.
	///
	///
	/// This queue is a ring buffer with wrap-around semantics with a tail and a head positions. The actual buffer index
	/// is the least significant bits of the tail and head positions.
	///
	/// @tparam T The type of the elements stored in the queue. It must be a trivial type (non trivial types could be
	/// implemented in the future).
	/// @tparam Size The size of the queue. It must be a power of 2 and less than 2^31.
	template <typename T, size_t Size>
	class StealingQueue
	{
		static_assert(Size > 0, "Size must be greater than 0");
		static_assert(Size < ((1ULL << (sizeof(uint32_t) * 8)) - 1), "Size must be less than 2^(31)");
		static_assert((Size & (Size - 1)) == 0, "Size must be a power of 2");

		static_assert(std::is_trivial_v<T>, "T must be a trivial type for StealingQueue to work correctly. You could "
											"file a bug for us to support non-trivial types in the future.");

	public:
		StealingQueue() noexcept
			: m_head(0),
			  m_tail(0),
			  m_data(),
			  m_mask(static_cast<uint32_t>(Size - 1))
		{
		}

		StealingQueue(const StealingQueue &) = delete;
		StealingQueue &operator=(const StealingQueue &) = delete;
		StealingQueue(StealingQueue &&) = delete;
		StealingQueue &operator=(StealingQueue &&) = delete;

		~StealingQueue() noexcept;

		/// Read an item at the given index in the queue.
		///
		/// This position is mapped to a valid ring buffer index using the mask.
		///
		/// For this to work correctly, the item must have been previously initialized at that index.
		///
		/// @param index The index to read from. It must be less than the capacity of the queue.
		/// @return A reference to the item at the given index.
		T &ReadAt(uint32_t index) noexcept;

		/// Write an item at the given index in the queue.
		///
		/// This position is mapped to a valid ring buffer index using the mask.
		///
		/// If a previous item was at that index, it will be overwritten and *not* destructed. So care must be taken.
		///
		/// @param index The index to write to.
		/// @param value The value to write at the given index. It must be a valid item.
		void WriteAt(uint32_t index, T &&value) noexcept;

		/// Attempt to mark a range of items for stealing. The amount of items is determined by
		/// the provided function `countFunc`, which is invoked with the number of items available in the queue.
		///
		/// In case of success, it returns a tuple containing the stealer's head index and the number of items. The
		/// number of items is guaranteed to be at least 1.
		///
		/// If the queue is empty, it returns `EStealError::Empty`. If the queue is busy (concurrent stealing
		/// operation), it returns `EStealError::Busy`.
		///
		/// @tparam F The type of the function that will be used to determine the number of items to steal. It must take
		/// a size_t and return a size_t.
		/// @param countFunc The function that will be used to determine the number of items to steal. It is invoked
		/// with the number of items available in the queue.
		/// @param maxCount The maximum number of items to steal. It is used to limit the number of items stolen in case
		/// the function returns a larger value than the available items.
		/// @return A Result containing a tuple with the stealer's head index and the number of items stolen, or an
		/// EStealError in case of failure.
		template <typename F>
			requires(std::is_invocable_r_v<size_t, F, size_t>)
		Result<std::tuple<uint32_t, uint32_t>, EStealError> MarkForSteal(F &countFunc, uint32_t maxCount);

		constexpr uint32_t Capacity() const noexcept
		{
			return m_mask + 1;
		}

	private:
		friend class Worker<T, Size>;
		friend class Stealer<T, Size>;

		/// Positions of the head as seen by the worker (most significant bits) and the stealer (least significant
		/// bits).
		alignas(std::hardware_destructive_interference_size) std::atomic<uint64_t> m_head;

		/// Position of the tail.
		alignas(std::hardware_destructive_interference_size) std::atomic<uint32_t> m_tail;

		/// The data buffer, which is a ring buffer. Static size is used to avoid dynamic memory allocation.
		std::array<T, Size> m_data;

		/// Mask to apply to the indices to wrap around the buffer. It is equal to Size - 1, which is a power of 2.
		uint32_t m_mask;
	};

	/// Handle for multi-threaded stealing operations.
	/// @tparam T The type of the items in the queue.
	/// @tparam Size The size of the queue. It must be a power of 2 and less than 2^31.
	template <typename T, size_t Size>
	class Stealer
	{
	public:
		explicit Stealer(std::shared_ptr<StealingQueue<T, Size>> workerQueue) noexcept;

		template <typename F>
			requires(std::is_invocable_r_v<size_t, F, size_t>)
		Result<std::tuple<uint32_t, uint32_t>, EStealError> Steal(Worker<T, Size> &dest, F &countFunc);

		template <typename F>
		Result<std::tuple<T, size_t>, EStealError> StealAndPop(Worker<T, Size> &dest, F countFunc);

	private:
		std::shared_ptr<StealingQueue<T, Size>> m_workerQueue;
	};

	/// Handle for single-threaded FIFO operations.
	/// @tparam T The type of the items in the queue.
	/// @tparam Size The size of the queue. It must be a power of 2 and less than 2^31.
	template <typename T, size_t Size>
	class Worker
	{
	public:
		Worker() noexcept;

		/// Creates a new stealer associated with this worker queue.
		///
		/// Any number of stealers can be created from a single worker queue.
		/// @return A new Stealer instance that can be used to steal items from this worker queue.
		[[nodiscard]]
		Stealer<T, Size> MakeStealer() noexcept;

		/// Returns the capacity of the worker queue.
		/// @return The capacity of the worker queue, which is equal to Size.
		[[nodiscard]]
		size_t Capacity() const noexcept;

		/// Returns the number of items that can be succesfully pushed onto the queue.
		///
		/// This number might not reflect the real number due to concurrent stealing operations.
		///
		/// @return The number of items that can be succesfully pushed onto the queue.
		[[nodiscard]]
		size_t SpareCapacity() const noexcept;

		/// Returns true if the worker queue is empty, false otherwise.
		///
		/// While the queue size might not be accurate, it is guaranteed that if a call to `IsEmpty()` returns true,
		/// a subsequent call to `Pop()` will fail.
		///
		/// @return True if the worker queue is empty, false otherwise.
		[[nodiscard]]
		bool IsEmpty() const noexcept;

		/// Push an item onto the worker queue.
		///
		/// @param value The value to push onto the queue. It must be a valid item.
		/// @return True if the item was successfully pushed onto the queue, false if the queue is full.
		bool Push(T &&value) noexcept;

		/// Attempt to pop an item from the head of the queue.
		///
		/// @return An empty optional if the queue is empty, or an optional containing the item if it was successfully
		/// popped.
		std::optional<T> Pop() noexcept;

	private:
		friend class Stealer<T, Size>;
		std::shared_ptr<StealingQueue<T, Size>> m_workerQueue;
	};

	template <typename T, size_t Size>
	StealingQueue<T, Size>::~StealingQueue() noexcept
	{
		if constexpr (std::is_destructible_v<T>)
		{
			const auto workerHead = std::get<0>(Unpack(m_head.load(std::memory_order::relaxed)));
			const auto tail = m_tail.load(std::memory_order::relaxed);

			const auto itemCount = tail - workerHead;
			for (uint32_t i = 0; i < itemCount; ++i)
			{
				auto &item = ReadAt(workerHead + i);
				item.~T();
			}
		}
	}

	template <typename T, size_t Size>
	T &StealingQueue<T, Size>::ReadAt(uint32_t index) noexcept
	{
		return m_data[index & m_mask];
	}

	template <typename T, size_t Size>
	void StealingQueue<T, Size>::WriteAt(uint32_t index, T &&value) noexcept
	{
		const uint32_t maskedIndex = index & m_mask;
		m_data[maskedIndex] = std::move(value);
	}

	template <typename T, size_t Size>
	template <typename F>
		requires(std::is_invocable_r_v<size_t, F, size_t>)
	Result<std::tuple<uint32_t, uint32_t>, EStealError> StealingQueue<T, Size>::MarkForSteal(F &countFunc,
																							 uint32_t maxCount)
	{
		auto heads = m_head.load(std::memory_order::acquire);

		while (true)
		{
			auto [workerHead, stealerHead] = Unpack(heads);

			if (stealerHead != workerHead)
			{
				return EStealError::Busy;
			}

			const auto tail = m_tail.load(std::memory_order::acquire);
			const auto itemCount = tail - workerHead;

			if (itemCount == 0)
			{
				return EStealError::Empty;
			}

			const auto count =
				std::min(static_cast<uint32_t>(std::min(countFunc(itemCount), static_cast<size_t>(maxCount))),
						 static_cast<uint32_t>(itemCount));

			if (count == 0)
			{
				return EStealError::Empty;
			}

			const auto newHeads = Pack(workerHead + count, stealerHead);

			if (m_head.compare_exchange_weak(heads, newHeads, std::memory_order::acquire, std::memory_order::acquire))
			{
				return std::make_tuple(stealerHead, count);
			}

			heads = m_head.load(std::memory_order::acquire);
		}
	}

	template <typename T, size_t Size>
	Stealer<T, Size>::Stealer(std::shared_ptr<StealingQueue<T, Size>> workerQueue) noexcept
		: m_workerQueue(std::move(workerQueue))
	{
	}

	template <typename T, size_t Size>
	template <typename F>
		requires(std::is_invocable_r_v<size_t, F, size_t>)
	Result<std::tuple<uint32_t, uint32_t>, EStealError> Stealer<T, Size>::Steal(Worker<T, Size> &dest, F &countFunc)
	{
		const auto destTail = dest.m_workerQueue->m_tail.load(std::memory_order::acquire);
		const auto destStealerHead = std::get<1>(Unpack(dest.m_workerQueue->m_head.load(std::memory_order::acquire)));
		const auto destFreeCapacity = dest.m_workerQueue->Capacity() - (destTail - destStealerHead);

		const auto stealResult = m_workerQueue->MarkForSteal(countFunc, static_cast<uint32_t>(destFreeCapacity));
		if (stealResult.has_error())
		{
			return stealResult.error();
		}
		const auto [stealerHead, transferCount] = stealResult.value();

		for (uint32_t i = 0; i < transferCount; ++i)
		{
			auto item = m_workerQueue->ReadAt(stealerHead + i);
			dest.m_workerQueue->WriteAt(destTail + i, std::move(item));
		}

		auto heads = m_workerQueue->m_head.load(std::memory_order::relaxed);

		while (true)
		{
			const auto [workerHead, stealerHead] = Unpack(heads);

			const auto packedResult = Pack(workerHead, workerHead);
			const auto res = m_workerQueue->m_head.compare_exchange_weak(
				heads, packedResult, std::memory_order::acq_rel, std::memory_order::acquire);

			if (res)
			{
				m_workerQueue->m_tail.store(destTail + transferCount, std::memory_order::release);
				return std::make_tuple(stealerHead, transferCount);
			}
			heads = m_workerQueue->m_head.load(std::memory_order::acquire);
		}
	}
	template <typename T, size_t Size>
	template <typename F>
	Result<std::tuple<T, size_t>, EStealError> Stealer<T, Size>::StealAndPop(Worker<T, Size> &dest, F countFunc)
	{
		const auto destTail = dest.m_workerQueue->m_tail.load(std::memory_order::relaxed);
		const auto destStealerHead = std::get<1>(Unpack(dest.m_workerQueue->m_head.load(std::memory_order::acquire)));
		const auto destFreeCapacity = dest.m_workerQueue->Capacity() - (destTail - destStealerHead);

		const auto stealResult = m_workerQueue->MarkForSteal(countFunc, static_cast<uint32_t>(destFreeCapacity));

		if (stealResult.has_error())
		{
			return stealResult.error();
		}

		const auto [stealerHead, count] = stealResult.value();
		const auto transferCount = count - 1;

		for (uint32_t i = 0; i < transferCount; ++i)
		{
			auto item = m_workerQueue->ReadAt(stealerHead + i);
			dest.m_workerQueue->WriteAt(destTail + i, std::move(item));
		}

		auto lastItem = std::move(m_workerQueue->ReadAt(static_cast<uint32_t>(stealerHead + transferCount)));

		dest.m_workerQueue->m_tail.store(static_cast<uint32_t>(destTail + transferCount), std::memory_order::release);

		auto heads = m_workerQueue->m_head.load(std::memory_order::relaxed);

		while (true)
		{
			const auto [workerHead, sh] = Unpack(heads);

			const auto packedResult = Pack(workerHead, workerHead);
			const auto res = m_workerQueue->m_head.compare_exchange_weak(
				heads, packedResult, std::memory_order::acq_rel, std::memory_order::acquire);

			if (res)
			{
				return std::make_tuple(std::move(lastItem), transferCount);
			}
			heads = m_workerQueue->m_head.load(std::memory_order::acquire);
		}
	}

	template <typename T, size_t Size>
	Worker<T, Size>::Worker() noexcept
		: m_workerQueue(std::make_shared<StealingQueue<T, Size>>())
	{
	}

	template <typename T, size_t Size>
	Stealer<T, Size> Worker<T, Size>::MakeStealer() noexcept
	{
		return Stealer{m_workerQueue};
	}

	template <typename T, size_t Size>
	size_t Worker<T, Size>::Capacity() const noexcept
	{
		return m_workerQueue->Capacity();
	}

	template <typename T, size_t Size>
	size_t Worker<T, Size>::SpareCapacity() const noexcept
	{
		const auto stealerHead = std::get<1>(Unpack(m_workerQueue->m_head.load(std::memory_order::relaxed)));
		const auto tail = m_workerQueue->m_tail.load(std::memory_order::relaxed);

		const auto length = tail - stealerHead;

		return m_workerQueue->Capacity() - length;
	}

	template <typename T, size_t Size>
	bool Worker<T, Size>::IsEmpty() const noexcept
	{
		const auto workerHead = std::get<0>(Unpack(m_workerQueue->m_head.load(std::memory_order::relaxed)));
		const auto tail = m_workerQueue->m_tail.load(std::memory_order::relaxed);

		return tail == workerHead;
	}

	template <typename T, size_t Size>
	bool Worker<T, Size>::Push(T &&value) noexcept
	{
		const auto stealerHead = std::get<1>(Unpack(m_workerQueue->m_head.load(std::memory_order::acquire)));
		const auto tail = m_workerQueue->m_tail.load(std::memory_order::relaxed);

		if ((tail - stealerHead) > m_workerQueue->m_mask)
		{
			return false; // Queue is full
		}

		m_workerQueue->WriteAt(tail, std::move(value));

		m_workerQueue->m_tail.store(tail + 1, std::memory_order::release);

		return true;
	}

	template <typename T, size_t Size>
	std::optional<T> Worker<T, Size>::Pop() noexcept
	{
		auto heads = m_workerQueue->m_head.load(std::memory_order::acquire);

		uint32_t prevWorkerHead{};

		while (true)
		{
			const auto [workerHead, stealerHead] = Unpack(heads);
			const auto tail = m_workerQueue->m_tail.load(std::memory_order::relaxed);

			if (tail == workerHead)
			{
				return std::nullopt; // Queue is empty
			}

			const auto nextHeads = Pack(workerHead + 1, stealerHead + (stealerHead == workerHead));

			if (m_workerQueue->m_head.compare_exchange_weak(heads, nextHeads, std::memory_order::acq_rel,
															std::memory_order::acquire))
			{
				prevWorkerHead = workerHead;
				break;
			}
			heads = m_workerQueue->m_head.load(std::memory_order::acquire);
		}

		return std::make_optional(std::move(m_workerQueue->ReadAt(prevWorkerHead)));
	}
} // namespace Hush::Threading
