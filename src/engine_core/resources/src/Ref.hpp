/*! \file Ref.hpp
	\author Kyn21kx
	\date 2025-06-27
	\brief Reference counted handler
*/

#pragma once

#include "IResourceManager.hpp"

namespace Hush
{
	// NOLINTBEGIN
	template<class T>
	class Ref {
	public:
		T* operator ->() const {
			T* result = reinterpret_cast<T*>(this->m_element);
			return result;
		}

		inline ~Ref() {
			this->m_resourceManager->DecreaseRefCount(this->m_element);
		}
		
		inline T* Get() {
			return reinterpret_cast<T*>(this->m_element);
		}

		inline bool IsNull() const {
			// TODO: Invalidate when the count reaches 0 even when in the middle of the frame
			const RefCounted& counter = this->m_resourceManager->GetRefCount(this->m_element);
			return this->m_element == INVALID_HANDLE || counter.element == nullptr || counter.count == 0;
		}

		Ref(IResourceManager* resourceManager, T* resource) {
			this->m_element = reinterpret_cast<HandleId>(resource);
			this->m_resourceManager = resourceManager;
			// Internally creates/increases the count at RefCounted for this handle
			RefCounted* count = this->m_resourceManager->IncreaseRefCount(this->m_element);
			count->deleter = [](void* ptr) {
				delete static_cast<T*>(ptr);
			};
		}

	private:
		HandleId m_element = INVALID_HANDLE;
		IResourceManager* m_resourceManager = nullptr;
	};
	// NOLINTEND
}
