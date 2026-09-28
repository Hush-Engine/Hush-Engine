/*! \file ResourceHandle.hpp
	\author Alan Ramirez Herrera
	\date 2025-11-24
	\brief RenderGraph implementation for rendering
*/

#pragma once

#include <type_traits>
#include <atomic>
#include <cstdint>
#include <memory>
#include <concepts>
#include "Assertions.hpp"

namespace Hush::Graphics
{
	class IGraphicsDevice;
	class IGraphicsBuffer;
	class IGraphicsTexture;
} // namespace Hush::Graphics

namespace Hush::RenderGraph
{
	class RenderGraph;

	template <typename T>
	concept ResourceConcept = requires(T a) {
		// T must have a nested type called Descriptor
		typename T::Descriptor;
		{
			a.CreateResource(std::declval<const typename T::Descriptor &>(),
							 static_cast<Graphics::IGraphicsDevice *>(nullptr))
		} -> std::same_as<void>;
		{
			a.DestroyResource(std::declval<const typename T::Descriptor &>(),
							  static_cast<Graphics::IGraphicsDevice *>(nullptr))
		} -> std::same_as<void>;
	} && std::is_default_constructible_v<T> && std::is_move_constructible_v<T>;

	class ResourceHandle
	{
		/// Represents the type of handle for a resource. A resource can either be imported from outside the graph
		/// (External), or created transiently inside of the graph (Transient).
		enum class EHandleType : uint8_t
		{
			/// Resource created and managed by the render graph
			Transient,
			/// Resource imported from outside the render graph, for instance, swapchain images
			External,
		};

		friend class RenderGraph;

	public:
		ResourceHandle() = delete;
		ResourceHandle(const ResourceHandle &) = delete;
		ResourceHandle(ResourceHandle &&) noexcept = default;
		~ResourceHandle() = default;

		ResourceHandle &operator=(const ResourceHandle &) = delete;
		ResourceHandle &operator=(ResourceHandle &&) noexcept = delete;

		[[nodiscard]]
		EHandleType GetHandleType() const noexcept
		{
			return m_handleType;
		}

		[[nodiscard]]
		uint32_t GetResourceId() const noexcept
		{
			return m_resourceId;
		}

		/// Unique for this resource instance, including replacements at the same address.
		[[nodiscard]]
		uint64_t GetInstanceId() const noexcept
		{
			return m_resourcePtr->instanceId;
		}

		/// Keeps graph-owned storage alive; imported raw RHI objects remain caller-owned.
		[[nodiscard]]
		std::shared_ptr<const void> GetLifetimeToken() const
		{
			return m_resourcePtr;
		}

		/// @brief Check whether the underlying GPU resource has been allocated.
		///
		/// During the render graph build phase, transient resources are declared
		/// but not yet created on the GPU. The executor calls CreateResource()
		/// during a realization step, after which this returns true.
		///
		/// External (imported) resources are always considered realized because
		/// they are created outside the graph and provided fully formed.
		[[nodiscard]]
		bool IsRealized() const noexcept
		{
			return m_realized;
		}

		/// @brief Create the underlying GPU resource and mark the handle as realized.
		///
		/// This should only be called once per handle (typically by the executor
		/// during its resource realization phase). Calling it on an already-realized
		/// handle is a no-op.
		///
		/// @param ctx The graphics device used to allocate the GPU resource.
		void CreateResource(Hush::Graphics::IGraphicsDevice *ctx)
		{
			if (m_realized)
			{
				return;
			}
			m_resourcePtr->CreateResource(ctx);
			m_realized = true;
		}

		void DestroyResource(Hush::Graphics::IGraphicsDevice *ctx)
		{
			m_resourcePtr->DestroyResource(ctx);
		}

		void BeforeRead(uint32_t flags, void *ctx)
		{
			m_resourcePtr->BeforeRead(flags, ctx);
		}

		void BeforeWrite(uint32_t flags, void *ctx)
		{
			m_resourcePtr->BeforeWrite(flags, ctx);
		}

		/// RHI buffer/texture pointer, NEVER the graph wrapper or native API handle.
		/// Dependency-only resources have no barrier object.
		[[nodiscard]]
		void *GetBarrierResource() const
		{
			return m_resourcePtr->GetBarrierResource();
		}

		[[nodiscard]]
		bool SupportsBarriers() const
		{
			return m_resourcePtr->SupportsBarriers();
		}

		template <ResourceConcept T>
		[[nodiscard]]
		const typename T::Descriptor &GetDescriptor() const
		{
			HUSH_ASSERT(m_resourcePtr != nullptr, "Resource pointer cannot be null!");
			const auto derivedPtr = dynamic_cast<ResourceModel<T> *>(m_resourcePtr.get());
			HUSH_ASSERT(derivedPtr != nullptr, "Failed to cast resource model to the requested type!");
			return derivedPtr->descriptor;
		}

		template <ResourceConcept T>
		T &GetResourceInstance()
		{
			HUSH_ASSERT(m_resourcePtr != nullptr, "Resource pointer cannot be null!");
			const auto derivedPtr = dynamic_cast<ResourceModel<T> *>(m_resourcePtr.get());
			HUSH_ASSERT(derivedPtr != nullptr, "Failed to cast resource model to the requested type!");
			return derivedPtr->resourceInstance;
		}

		/// Replace an import without mutating storage retained by submitted work.
		/// Use RenderGraph::UpdateImport to also establish its incoming state.
		template <ResourceConcept T>
		[[nodiscard]]
		bool UpdateExternalResource(T &&newResource)
		{
			auto *model = dynamic_cast<ResourceModel<T> *>(m_resourcePtr.get());
			if (m_handleType != EHandleType::External || model == nullptr)
			{
				return false;
			}
			m_resourcePtr = std::make_shared<ResourceModel<T>>(model->descriptor, std::forward<T>(newResource));
			return true;
		}

	private:
		template <ResourceConcept T>
		ResourceHandle(const typename T::Descriptor &descriptor, T &&resourceInstance, EHandleType handleType,
					   uint32_t resourceId)
			: m_resourcePtr(std::make_shared<ResourceModel<T>>(descriptor, std::forward<T>(resourceInstance))),
			  m_handleType(handleType),
			  m_resourceId(resourceId),
			  m_realized(handleType == EHandleType::External)
		{
		}

	private:
		class IResourceModel
		{
		public:
			IResourceModel() = default;
			IResourceModel(const IResourceModel &) = delete;
			IResourceModel(IResourceModel &&) = delete;
			IResourceModel &operator=(const IResourceModel &) = delete;
			IResourceModel &operator=(IResourceModel &&) = delete;

			virtual ~IResourceModel() = default;

			const uint64_t instanceId = NEXT_INSTANCE_ID.fetch_add(1, std::memory_order_relaxed);

			virtual void CreateResource(Hush::Graphics::IGraphicsDevice *ctx) = 0;

			virtual void DestroyResource(Hush::Graphics::IGraphicsDevice *ctx) = 0;

			virtual void BeforeRead(uint32_t flags, void *ctx) = 0;

			virtual void BeforeWrite(uint32_t flags, void *ctx) = 0;

			[[nodiscard]]
			virtual const void *GetDescriptorPtr() const = 0;

			[[nodiscard]]
			virtual void *GetBarrierResource() const = 0;
			[[nodiscard]]
			virtual bool SupportsBarriers() const = 0;
		};

		template <ResourceConcept T>
		struct ResourceModel : public IResourceModel
		{
			static constexpr bool HAS_BEFORE_READ = requires(T a, uint32_t flags, void *ctx) {
				{ a.BeforeRead(flags, ctx) } -> std::same_as<void>;
			};

			static constexpr bool HAS_BEFORE_WRITE = requires(T a, uint32_t flags, void *ctx) {
				{ a.BeforeWrite(flags, ctx) } -> std::same_as<void>;
			};

		public:
			ResourceModel() = default;

			ResourceModel(const ResourceModel &) = delete;
			ResourceModel(ResourceModel &&) = delete;
			ResourceModel &operator=(const ResourceModel &) = delete;
			ResourceModel &operator=(ResourceModel &&) = delete;

			ResourceModel(const typename T::Descriptor &descriptor, T &&obj)
				: resourceInstance(std::move(obj)),
				  descriptor(descriptor)
			{
			}

			~ResourceModel() override
			{
				DestroyResource(m_creator);
			}

			void CreateResource(Hush::Graphics::IGraphicsDevice *ctx) override
			{
				resourceInstance.CreateResource(descriptor, ctx);
				m_creator = ctx;
			}

			void DestroyResource(Hush::Graphics::IGraphicsDevice *ctx) override
			{
				if (m_creator != nullptr)
				{
					resourceInstance.DestroyResource(descriptor, ctx);
					m_creator = nullptr;
				}
			}

			void BeforeRead(uint32_t flags, void *ctx) override
			{
				if constexpr (HAS_BEFORE_READ)
				{
					resourceInstance.BeforeRead(flags, ctx);
				}
			}

			void BeforeWrite(uint32_t flags, void *ctx) override
			{
				if constexpr (HAS_BEFORE_WRITE)
				{
					resourceInstance.BeforeWrite(flags, ctx);
				}
			}

			[[nodiscard]]
			const void *GetDescriptorPtr() const override
			{
				return &descriptor;
			}

			static constexpr bool HAS_BARRIER_RESOURCE = requires(const T &resource) {
				{ resource.GetBarrierResource() } -> std::same_as<Graphics::IGraphicsBuffer *>;
			} || requires(const T &resource) {
				{ resource.GetBarrierResource() } -> std::same_as<Graphics::IGraphicsTexture *>;
			};

			[[nodiscard]]
			void *GetBarrierResource() const override
			{
				if constexpr (HAS_BARRIER_RESOURCE)
				{
					return resourceInstance.GetBarrierResource();
				}
				else
				{
					return nullptr;
				}
			}

			[[nodiscard]]
			bool SupportsBarriers() const override
			{
				return HAS_BARRIER_RESOURCE;
			}

			T resourceInstance;
			const typename T::Descriptor descriptor;

		private:
			Graphics::IGraphicsDevice *m_creator = nullptr;
		};

	private:
		inline static std::atomic<uint64_t> NEXT_INSTANCE_ID{1};
		std::shared_ptr<IResourceModel> m_resourcePtr;

		const EHandleType m_handleType;
		const uint32_t m_resourceId{};

		/// Whether the underlying GPU resource has been allocated.
		/// External resources start as realized; transient resources start as
		/// unrealized and are created during the executor's realization phase.
		bool m_realized = false;
	};
} // namespace Hush::RenderGraph
