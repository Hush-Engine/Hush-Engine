
#pragma once

#include "ResourceId.hpp"
#include "Shared/MaterialOptions.hpp"

#include <vector>
#include <string_view>
#include <string>
#include <memory>

namespace Hush::RenderGraph
{
	/// Base class for a pass execution context. This is meant to be stored as the callable for each pass node.
	struct PassBase
	{
		PassBase() = default;
		virtual ~PassBase() = default;

		PassBase(const PassBase &) = delete;
		PassBase &operator=(const PassBase &) = delete;
		PassBase(PassBase &&) = delete;
		PassBase &operator=(PassBase &&) = delete;

		virtual void Execute(void *ctx) = 0;
	};

	/// A typed callable pass inside of the render graph.
	/// @tparam PassData The type of local data for this pass, this can be any struct or class.
	/// @tparam BuildFn The type of the build function, must be callable as:
	///     void(BuildContext &, PassData &)
	/// @tparam ExecuteFn The type of the execute function, must be callable as:
	///     void(PassData &, RenderGraphResources &, IRenderContext *, ICommandQueue*)
	template <typename PassData, typename ExecuteFn>
		requires(std::is_invocable_r_v<void, ExecuteFn, PassData &, void *>)
	class Pass : public PassBase
	{
	public:
		Pass(ExecuteFn &&e)
			: execFn(std::move(e))
		{
		}

		void Execute(void *ctx) override
		{
			HUSH_ASSERT(ctx != nullptr, "Render context cannot be null during execution!");
			HUSH_ASSERT(execFn != nullptr, "Execute function cannot be null during execution!");

			// TODO: Depending on passType, we might need to get a different command queue from renderCtx
			// ICommandQueue *cmdQueue = nullptr;
			// cmdQueue = renderCtx->GetCommandQueueForPassType(passType);

			execFn(data, ctx);
		}

		ExecuteFn execFn;
		PassData data;
	};

	/// Represents a single pass inside of the render graph
	class RenderPassNode final
	{
		friend class RenderGraph;
		enum class EPassAccessFlags : uint8_t
		{
			None = 0,
			Read = 1 << 0,
			Write = 1 << 1,
		};

		struct PassAccess
		{
			ResourceId resourceId{};
		};

	public:
		enum class EPassCullingMode : uint8_t
		{
			CullIfPossible,
			NeverCull,
		};

		[[nodiscard]]
		bool ReadsResource(ResourceId id) const;

		[[nodiscard]]
		bool WritesResource(ResourceId id) const;

		[[nodiscard]]
		bool CreatesResource(ResourceId id) const;

		[[nodiscard]]
		EPassCullingMode GetCullingMode() const noexcept
        {
            return m_cullingMode;
        }

	private:
		RenderPassNode(std::string_view name, uint32_t nodeId, std::unique_ptr<PassBase> &&pass);

		void AddCreatedResource(ResourceId id);

		[[nodiscard]]
		ResourceId AddReadResource(ResourceId id);

		[[nodiscard]]
		ResourceId AddWrittenResource(ResourceId id);

	private:
		void Execute(void *ctx)
		{
			m_pass->Execute(ctx);
		}

		std::string m_name;

		std::vector<ResourceId> m_createsResources;

		std::vector<PassAccess> m_readResources;
		std::vector<ResourceId> m_writtenResources;

		/// The pass execution context.
		std::unique_ptr<PassBase> m_pass;

		const uint32_t m_nodeId;
		int32_t m_refCount{};
		EPassCullingMode m_cullingMode = EPassCullingMode::CullIfPossible;
	};
} // namespace Hush::RenderGraph
