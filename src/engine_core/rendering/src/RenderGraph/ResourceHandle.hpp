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

namespace Hush::Graphics
{
	class IGraphicsDevice;
}

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
		static constexpr uint32_t RESOURCE_INITIAL_VERSION = 1;

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

		/// @brief Get an opaque pointer to the underlying native GPU resource.
		///
		/// Used by the render graph's barrier computation to populate
		/// ResourceBarrierDescriptor::resource. The actual type depends on the
		/// concrete resource (e.g. ID3D12Resource*, VkImage, wgpu::Texture, etc.).
		///
		/// @return Opaque pointer to the native resource, or nullptr if not yet created.
		[[nodiscard]]
		void *GetNativePtr() const
		{
			if (m_resourcePtr)
			{
				return m_resourcePtr->GetNativePtr();
			}
			return nullptr;
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

		/// @brief Replace the resource instance of an External (imported) handle in-place.
		///
		/// This is used to update per-frame imported resources (e.g. the current
		/// swapchain backbuffer) without tearing down and rebuilding the entire
		/// render graph.  The graph topology, compilation state, and resource IDs
		/// all remain unchanged — only the underlying data pointer is swapped.
		///
		/// @pre The handle must be External (imported).  Calling this on a
		///      Transient handle is a logic error and will assert.
		/// @tparam T Must satisfy ResourceConcept and match the type originally
		///           used when the resource was imported.
		/// @param newResource The new resource instance to move into the handle.
		template <ResourceConcept T>
		void UpdateExternalResource(T &&newResource)
		{
			HUSH_ASSERT(m_handleType == EHandleType::External,
						"UpdateExternalResource can only be called on imported (External) handles!");
			HUSH_ASSERT(m_resourcePtr != nullptr, "Resource pointer cannot be null!");

			auto *derivedPtr = dynamic_cast<ResourceModel<T> *>(m_resourcePtr.get());
			HUSH_ASSERT(derivedPtr != nullptr, "Type mismatch: UpdateExternalResource<T> called with a different T "
											   "than the one used at import time!");

			derivedPtr->resourceInstance = std::forward<T>(newResource);
		}

	private:
		template <ResourceConcept T>
		ResourceHandle(const typename T::Descriptor &descriptor, T &&resourceInstance, EHandleType handleType,
					   uint32_t resourceId)
			: m_resourcePtr(std::make_unique<ResourceModel<T>>(descriptor, std::forward<T>(resourceInstance))),
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
			IResourceModel(const IResourceModel &) = default;
			IResourceModel(IResourceModel &&) = default;
			IResourceModel &operator=(const IResourceModel &) = default;
			IResourceModel &operator=(IResourceModel &&) = delete;

			virtual ~IResourceModel() = default;

			virtual void CreateResource(Hush::Graphics::IGraphicsDevice *ctx) = 0;

			virtual void DestroyResource(Hush::Graphics::IGraphicsDevice *ctx) = 0;

			virtual void BeforeRead(uint32_t flags, void *ctx) = 0;

			virtual void BeforeWrite(uint32_t flags, void *ctx) = 0;

			[[nodiscard]]
			virtual const void *GetDescriptorPtr() const = 0;

			/// @brief Get an opaque pointer to the underlying native GPU resource.
			/// @return Pointer to the native resource, or nullptr if unavailable.
			[[nodiscard]]
			virtual void *GetNativePtr() const = 0;
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
			{
				new (const_cast<typename T::Descriptor*>(&this->descriptor)) typename T::Descriptor(descriptor);
				this->resourceInstance = std::move(obj);
			}

			~ResourceModel() override = default;

			void CreateResource(Hush::Graphics::IGraphicsDevice *ctx) override
			{
				resourceInstance.CreateResource(descriptor, ctx);
			}

			void DestroyResource(Hush::Graphics::IGraphicsDevice *ctx) override
			{
				resourceInstance.DestroyResource(descriptor, ctx);
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

			[[nodiscard]]
			void *GetNativePtr() const override
			{
				// Return the address of the resource instance itself as an opaque
				// pointer. Concrete resource types (e.g. a Vulkan texture wrapper)
				// can override or expose their own native handle through their type.
				return const_cast<T *>(&resourceInstance);
			}

			T resourceInstance;
			const typename T::Descriptor descriptor;
		};

	private:
		std::unique_ptr<IResourceModel> m_resourcePtr;

		const EHandleType m_handleType;
		const uint32_t m_resourceId{};
		uint32_t m_version = 0;

		/// Whether the underlying GPU resource has been allocated.
		/// External resources start as realized; transient resources start as
		/// unrealized and are created during the executor's realization phase.
		bool m_realized = false;
	};
} // namespace Hush::RenderGraph
