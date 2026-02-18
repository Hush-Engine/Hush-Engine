/*! \file ResourceHandle.hpp
	\author Alan Ramirez Herrera
	\date 2025-11-24
	\brief RenderGraph implementation for rendering
*/

#pragma once

#include <type_traits>
#include <memory>
#include <concepts>
#include "Assertions.hpp"
#include <string_view>

namespace Hush::RenderGraph
{
    class RenderPassNode;


	template <typename T>
	concept ResourceConcept = requires(T a) {
		// T must have a nested type called Descriptor
		typename T::Descriptor;
		{
			a.CreateResource(std::declval<const typename T::Descriptor &>(), static_cast<void *>(nullptr))
		} -> std::same_as<void>;
		{
			a.DestroyResource(std::declval<const typename T::Descriptor &>(), static_cast<void *>(nullptr))
		} -> std::same_as<void>;

		std::is_default_constructible_v<T>;
		std::is_move_constructible_v<T>;
	};

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
	    static constexpr uint32_t RESOURCE_INITIAL_VERSION = 1;


		ResourceHandle() = delete;
		ResourceHandle(const ResourceHandle&) = delete;
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

		void CreateResource(void *ctx)
           {
               m_resourcePtr->CreateResource(ctx);
           }

           void DestroyResource(void *ctx)
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
           T& GetResourceInstance()
           {
               HUSH_ASSERT(m_resourcePtr != nullptr, "Resource pointer cannot be null!");
               const auto derivedPtr = dynamic_cast<ResourceModel<T> *>(m_resourcePtr.get());
               HUSH_ASSERT(derivedPtr != nullptr, "Failed to cast resource model to the requested type!");
               return derivedPtr->resourceInstance;
           }

       private:
           template <ResourceConcept T>
           ResourceHandle(const typename T::Descriptor &descriptor, T &&resourceInstance,
                          EHandleType handleType, uint32_t resourceId)
               : m_resourcePtr(std::make_unique<ResourceModel<T>>(descriptor, std::forward(resourceInstance))),
                 m_handleType(handleType),
                 m_resourceId(resourceId)
           {
           }

	private:
		class IResourceModel
		{
		public:
			IResourceModel(const IResourceModel &) = default;
			IResourceModel(IResourceModel &&) = default;
			IResourceModel &operator=(const IResourceModel &) = default;
			IResourceModel &operator=(IResourceModel &&) = delete;

			virtual ~IResourceModel() = default;

			virtual void CreateResource(void *ctx) = 0;

			virtual void DestroyResource(void *ctx) = 0;

			virtual void BeforeRead(uint32_t flags, void *ctx) = 0;

			virtual void BeforeWrite(uint32_t flags, void *ctx) = 0;

			[[nodiscard]]
			virtual const void *GetDescriptorPtr() const = 0;
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
				: descriptor(descriptor),
				  resourceInstance(std::move(obj))
			{
			}

			~ResourceModel() override = default;

			void CreateResource(void *ctx) override
			{
				resourceInstance.CreateResource(descriptor, ctx);
			}

			void DestroyResource(void *ctx) override
			{
				resourceInstance.DestroyResource(descriptor, ctx);
			}

			void BeforeRead(uint32_t flags, void *ctx) override
			{
				if constexpr (HAS_BEFORE_READ)
				{
					static_cast<T *>(this)->BeforeRead(flags, ctx);
				}
			}

			void BeforeWrite(uint32_t flags, void *ctx) override
			{
				if constexpr (HAS_BEFORE_WRITE)
				{
					static_cast<T *>(this)->BeforeWrite(flags, ctx);
				}
			}

			[[nodiscard]]
			const void *GetDescriptorPtr() const override
			{
				return &descriptor;
			}

			T resourceInstance;
			const typename T::Descriptor descriptor;
		};

	private:
		std::unique_ptr<IResourceModel> m_resourcePtr;

		RenderPassNode *m_producer = nullptr;
		RenderPassNode *m_lastConsumer = nullptr;

		const EHandleType m_handleType;
		const uint32_t m_resourceId{};
		uint32_t m_version = 0;
	};
}
