
/*! \file Ref.hpp
	\author Kyn21kx
	\date 2025-06-27
	\brief Reference counted handler
*/

#pragma once

#include <cstdint>

#include "IResourceManager.hpp"

namespace Hush
{
	// NOLINTBEGIN
	template <class T>
	class Ref
	{
	public:
		T *operator->() const
		{
			T *result = reinterpret_cast<T *>(this->m_element);
			return result;
		}

		inline ~Ref()
		{
			if (this->IsNull())
				return;
			this->m_resourceManager->DecreaseRefCount(this->m_element);
		}

		inline T *Get()
		{
			return reinterpret_cast<T *>(this->m_element);
		}

		inline const T *Get() const
		{
			return reinterpret_cast<const T *>(this->m_element);
		}

		inline bool IsNull() const
		{
			// TODO: Invalidate when the count reaches 0 even when in the middle of the frame
			return this->m_element == INVALID_HANDLE || this->m_resourceManager->GetRefCount(this->m_element).IsNull();
		}

		/// @brief Fnv1a64 hash of the identifier the resource is stored under in the resource manager.
		/// Zero when the reference was created without an identifier.
		[[nodiscard]]
		uint64_t GetResourceId() const
		{
			return this->m_resourceId;
		}

		Ref() = default;

		Ref(const Ref &other)
			: m_resourceId(other.m_resourceId),
			  m_element(other.m_element),
			  m_resourceManager(other.m_resourceManager)
		{
			m_resourceManager->IncreaseRefCount(m_element);
		}

		Ref(Ref &&other) noexcept
			: m_resourceId(other.m_resourceId),
			  m_element(other.m_element),
			  m_resourceManager(other.m_resourceManager)
		{
			// Invalidate source to prevent decrement on destruction
			other.m_resourceId = 0;
			other.m_element = INVALID_HANDLE;
			other.m_resourceManager = nullptr;
		}

		Ref &operator=(const Ref &other)
		{
			if (this == &other)
			{
				return *this;
			}

			HandleId previousElement = this->m_element;
			IResourceManager *previousMananger = this->m_resourceManager;

			// We're assigning to ourselves, do not increase
			if (previousMananger == other.m_resourceManager && previousElement == other.m_element)
			{
				return *this;
			}

			// Only increase ref count if source is valid (not scheduled for deletion)
			if (!other.IsNull())
			{ // We actually sort of need to test this tbh
				other.m_resourceManager->IncreaseRefCount(other.m_element);

				this->m_element = other.m_element;
				this->m_resourceManager = other.m_resourceManager;
			}
			else
			{
				this->m_element = INVALID_HANDLE;
				this->m_resourceManager = nullptr;
			}

			this->m_resourceId = other.m_resourceId;

			// Release previous resource
			if (previousElement != INVALID_HANDLE && previousMananger != nullptr)
			{
				previousMananger->DecreaseRefCount(previousElement);
			}

			return *this;
		}

		Ref &operator=(Ref &&other) noexcept
		{
			if (this == &other)
			{
				return *this;
			}

			HandleId previousElement = this->m_element;
			IResourceManager *previousMananger = this->m_resourceManager;

			this->m_resourceId = other.m_resourceId;
			this->m_element = other.m_element;
			this->m_resourceManager = other.m_resourceManager;

			// Invalidate source to prevent decrement on destruction
			other.m_resourceId = 0;
			other.m_element = INVALID_HANDLE;
			other.m_resourceManager = nullptr;

			// Release previous resource
			if (previousElement != INVALID_HANDLE && previousMananger != nullptr)
			{
				previousMananger->DecreaseRefCount(previousElement);
			}

			return *this;
		}

		Ref(IResourceManager *resourceManager, T *resource)
			: Ref(resourceManager, resource, 0)
		{
		}

		Ref(IResourceManager *resourceManager, T *resource, uint64_t resourceId)
			: m_resourceId(resourceId),
			  m_element(reinterpret_cast<HandleId>(resource)),
			  m_resourceManager(resourceManager)
		{
			// Internally creates/increases the count at RefCounted for this handle
			RefCounted *count = this->m_resourceManager->IncreaseRefCount(this->m_element);
			count->element = static_cast<void *>(resource);
			if (count->deleter == nullptr)
			{
				count->deleter = [](void *ptr) { delete static_cast<T *>(ptr); };
			}
		}

	private:
		uint64_t m_resourceId = 0;
		HandleId m_element = INVALID_HANDLE;
		IResourceManager *m_resourceManager = nullptr;
	};
	// NOLINTEND
} // namespace Hush
