/*! \file CoroutineFrameResource.cpp
	\author Alan Ramirez Herrera
	\date 2026-07-01
	\brief Thread-sharded free-list backing the coroutine frame memory resource.

	Each thread owns a shard of size-classed free-lists. Same-thread allocate/deallocate is
	lock- and atomic-free (the common ParallelFor case: a frame allocated and freed on the same
	worker). A frame freed by a different thread than allocated it is pushed onto the owning
	shard's atomic MPSC "foreign free" list, which the owner reclaims in one swap when it next
	needs a block of that size class. Shards and slabs are intentionally leaked (process
	lifetime), so a frame freed after its allocating thread exits is still valid.
*/

#include "CoroutineFrameResource.hpp"

#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>

namespace Hush::Memory
{
	namespace
	{
		// The alignment we serve. The payload sits HEADER_SIZE bytes after the block start,
		// which holds the header (when allocated) or the free-list node (when free).
		constexpr std::size_t ALIGNMENT = __STDCPP_DEFAULT_NEW_ALIGNMENT__;

		// Size classes are TOTAL block sizes (header + payload). In an optimized build
		// the hot Task/ParallelFor/WhenAll frames are ~96/128/160 B of payload (~112/144/176 B
		// total, >99.9% of allocations), so the classes are dense at 128/160/192 (each hot total
		// rounds up only ~16 B) and coarse away from there. Rare tiny/large frames tolerate the
		// extra rounding, per the "prioritize the common sizes" trade-off. Every size is a
		// multiple of ALIGNMENT so cells carved from an aligned slab stay aligned.
		constexpr std::size_t CELL_SIZES[] = {32, 64, 128, 160, 192, 256, 384, 512, 1024};
		constexpr int NUM_SIZE_CLASSES = static_cast<int>(sizeof(CELL_SIZES) / sizeof(CELL_SIZES[0]));
		constexpr std::size_t MAX_CELL = CELL_SIZES[NUM_SIZE_CLASSES - 1];

		// O(1) size->class map: a table over [0, MAX_CELL] at ALIGNMENT granularity. Bucket
		// b = totals in (b*ALIGNMENT, (b+1)*ALIGNMENT]; CLASS_TABLE[b] is the smallest class whose
		// cell fits that bucket's largest total. Works for arbitrary (non-power-of-two) classes.
		constexpr int NUM_BUCKETS = static_cast<int>(MAX_CELL / ALIGNMENT);
		constexpr std::array<std::uint8_t, static_cast<std::size_t>(NUM_BUCKETS)> BuildClassTable()
		{
			std::array<std::uint8_t, static_cast<std::size_t>(NUM_BUCKETS)> table{};
			for (int b = 0; b < NUM_BUCKETS; ++b)
			{
				const std::size_t target = static_cast<std::size_t>(b + 1) * ALIGNMENT;
				int cls = 0;
				while (cls < NUM_SIZE_CLASSES && CELL_SIZES[cls] < target)
				{
					++cls;
				}
				table[static_cast<std::size_t>(b)] = static_cast<std::uint8_t>(cls);
			}
			return table;
		}
		constexpr auto CLASS_TABLE = BuildClassTable();

		static_assert(CELL_SIZES[0] % ALIGNMENT == 0);
		static_assert(MAX_CELL % ALIGNMENT == 0);

		constexpr std::size_t SLAB_BYTES = std::size_t{64} * 1024; // upstream request per refill
		constexpr std::size_t MIN_SLAB_CELLS = 8;

		constexpr std::size_t AlignUp(std::size_t n, std::size_t a) noexcept
		{
			return (n + a - 1) & ~(a - 1);
		}

		struct FreeNode
		{
			FreeNode *next;
		};

		struct Shard
		{
			FreeNode *localFree[NUM_SIZE_CLASSES] = {};					// owning thread only
			std::atomic<FreeNode *> foreignFree[NUM_SIZE_CLASSES] = {}; // pushed by other threads (MPSC)
		};

		// Header prepended to every allocation. `owner == nullptr` marks a large block that came
		// straight from upstream; otherwise the block belongs to `owner`'s free-list `classIndex`.
		struct BlockHeader
		{
			Shard *owner;
			std::uint32_t classIndex;
			std::uint32_t blockSize; // total bytes handed to upstream (large blocks only)
		};

		// Rounded up so the payload (block + HEADER_SIZE) keeps ALIGNMENT.
		constexpr std::size_t HEADER_SIZE = AlignUp(sizeof(BlockHeader), ALIGNMENT);

		static_assert(sizeof(FreeNode) <= HEADER_SIZE);
		static_assert(CELL_SIZES[0] >= HEADER_SIZE);

		// Smallest class whose cell fits `totalBytes`, or -1 if it exceeds MAX_CELL (-> upstream).
		// O(1) table lookup.
		int SizeClassIndex(std::size_t totalBytes) noexcept
		{
			if (totalBytes > MAX_CELL)
			{
				return -1;
			}
			return CLASS_TABLE[(totalBytes - 1) / ALIGNMENT];
		}

		// The calling thread's shard pointer (null until first allocation on this thread). The
		// pointed-to Shard is heap-allocated and leaked, so it outlives the thread — foreign
		// frees remain valid after the allocating thread exits.
		Shard *&LocalShardSlot() noexcept
		{
			thread_local Shard *shard = nullptr;
			return shard;
		}

		Shard &LocalShard()
		{
			Shard *&slot = LocalShardSlot();
			if (slot == nullptr)
			{
				slot = new Shard();
			}
			return *slot;
		}

		/// The coroutine-frame resource: a thin `memory_resource` over the thread-sharded pools.
		class ShardedCoroutineResource final : public std::pmr::memory_resource
		{
		public:
			explicit ShardedCoroutineResource(std::pmr::memory_resource *upstream) noexcept
				: m_upstream(upstream)
			{
			}

		protected:
			void *do_allocate(std::size_t bytes, std::size_t alignment) override
			{
				// Coroutine frames never over-align; keep the header scheme simple.
				assert(alignment <= ALIGNMENT);
				(void)alignment;

				const std::size_t total = HEADER_SIZE + bytes;
				const int sc = SizeClassIndex(total);
				if (sc < 0)
				{
					// Too big to pool: prefix a header and hand the rest back from upstream.
					const std::size_t blockSize = AlignUp(total, ALIGNMENT);
					void *base = m_upstream->allocate(blockSize, ALIGNMENT);
					::new (base) BlockHeader{nullptr, 0U, static_cast<std::uint32_t>(blockSize)};
					return static_cast<std::byte *>(base) + HEADER_SIZE;
				}

				Shard &shard = LocalShard();
				if (shard.localFree[sc] == nullptr)
				{
					// Reclaim everything other threads freed for this class in one swap...
					shard.localFree[sc] = shard.foreignFree[sc].exchange(nullptr, std::memory_order_acquire);
					if (shard.localFree[sc] == nullptr)
					{
						// ...still empty: carve a fresh slab.
						RefillFromSlab(shard, sc);
					}
				}

				FreeNode *node = shard.localFree[sc];
				shard.localFree[sc] = node->next;
				::new (node)
					BlockHeader{&shard, static_cast<std::uint32_t>(sc), static_cast<std::uint32_t>(CELL_SIZES[sc])};
				return reinterpret_cast<std::byte *>(node) + HEADER_SIZE;
			}

			void do_deallocate(void *ptr, std::size_t /*bytes*/, std::size_t /*alignment*/) noexcept override
			{
				auto *header = reinterpret_cast<BlockHeader *>(static_cast<std::byte *>(ptr) - HEADER_SIZE);
				Shard *owner = header->owner;
				if (owner == nullptr)
				{
					m_upstream->deallocate(header, header->blockSize, ALIGNMENT);
					return;
				}

				const std::uint32_t sc = header->classIndex;
				auto *node = reinterpret_cast<FreeNode *>(header); // reuse the block as a free node

				if (owner == LocalShardSlot())
				{
					// Same-thread free: no atomics.
					node->next = owner->localFree[sc];
					owner->localFree[sc] = node;
				}
				else
				{
					// Cross-thread free: push onto the owner's MPSC foreign list.
					FreeNode *head = owner->foreignFree[sc].load(std::memory_order_relaxed);
					do
					{
						node->next = head;
					} while (!owner->foreignFree[sc].compare_exchange_weak(head, node, std::memory_order_release,
																		   std::memory_order_relaxed));
				}
			}

			[[nodiscard]]
			bool do_is_equal(const std::pmr::memory_resource &other) const noexcept override
			{
				return this == &other;
			}

		private:
			void RefillFromSlab(Shard &shard, int sc)
			{
				const std::size_t cell = CELL_SIZES[sc];
				std::size_t cells = SLAB_BYTES / cell;
				if (cells < MIN_SLAB_CELLS)
				{
					cells = MIN_SLAB_CELLS;
				}
				auto *slab = static_cast<std::byte *>(m_upstream->allocate(cell * cells, ALIGNMENT));
				for (std::size_t i = 0; i < cells; ++i)
				{
					auto *node = reinterpret_cast<FreeNode *>(slab + i * cell);
					node->next = shard.localFree[sc];
					shard.localFree[sc] = node;
				}
				// The slab is intentionally leaked (process lifetime).
			}

			std::pmr::memory_resource *m_upstream;
		};
	} // namespace

	std::pmr::memory_resource *CoroutineFrameResource() noexcept
	{
		// Leaked singleton: coroutine operator new/delete may run during static de-initialization,
		// so the resource (and every shard/slab it owns) must live for the whole process.
		static auto *resource = new ShardedCoroutineResource(std::pmr::new_delete_resource());
		return resource;
	}
} // namespace Hush::Memory
