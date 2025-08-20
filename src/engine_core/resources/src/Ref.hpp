
/*! \file Ref.hpp
	\author Kyn21kx
	\date 2025-06-27
	\brief Reference counted handler
*/

#pragma once

#include "IResourceManager.hpp"
#include "Logger.hpp"

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
			if (this->IsNull()) return;
			RefCounted* count = this->m_resourceManager->DecreaseRefCount(this->m_element);
			LogFormat(ELogLevel::Info, "Decreased ref count of {} to: {}", this->m_element, count->count.load());
		}
		
		inline T* Get() {
			return reinterpret_cast<T*>(this->m_element);
		}
		
		inline const T* Get() const {
			return reinterpret_cast<const T*>(this->m_element);
		}

		inline bool IsNull() const {
			// TODO: Invalidate when the count reaches 0 even when in the middle of the frame
			return this->m_element == INVALID_HANDLE || this->m_resourceManager->GetRefCount(this->m_element).IsNull();
		}		

		Ref() = default;
		
		Ref(const Ref& other) : 
            m_element(other.m_element),
            m_resourceManager(other.m_resourceManager) 
        {
            RefCounted* count = m_resourceManager->IncreaseRefCount(m_element);
            LogFormat(ELogLevel::Info, "Increased ref count of {} to: {}", m_element, count->count.load());
        }

        Ref(Ref&& other) noexcept : 
            m_element(other.m_element),
            m_resourceManager(other.m_resourceManager) 
        {
            // Invalidate source to prevent decrement on destruction
            other.m_element = INVALID_HANDLE;
            other.m_resourceManager = nullptr;
        }

        
		Ref& operator=(const Ref& other) {
		    if (this == &other) {
		        return *this;
		    }

		    HandleId previousElement = this->m_element;
		    IResourceManager* previousMananger = this->m_resourceManager;

		    // Only increase ref count if source is valid (not scheduled for deletion)
		    if (!other.IsNull()) { // We actually sort of need to test this tbh
		        other.m_resourceManager->IncreaseRefCount(other.m_element);
        
		        this->m_element = other.m_element;
		        this->m_resourceManager = other.m_resourceManager;
		    }
		    else {
		        this->m_element = INVALID_HANDLE;
		        this->m_resourceManager = nullptr;
		    }

		    // Release previous resource
		    if (previousElement != INVALID_HANDLE && previousMananger != nullptr) {
		        previousMananger->DecreaseRefCount(previousElement);
		    }

		    return *this;
		}		
		
		Ref(IResourceManager* resourceManager, T* resource) {
			this->m_element = reinterpret_cast<HandleId>(resource);
			this->m_resourceManager = resourceManager;
			// Internally creates/increases the count at RefCounted for this handle
			RefCounted* count = this->m_resourceManager->IncreaseRefCount(this->m_element);
			count->element = static_cast<void*>(resource);
			LogFormat(ELogLevel::Info, "Increased ref count of {} to: {}", this->m_element, count->count.load());
			if (count->deleter == nullptr) {
				count->deleter = [](void* ptr) {
					LogFormat(ELogLevel::Info, "Deleted reference with ID: {}", reinterpret_cast<HandleId>(ptr));
					delete static_cast<T*>(ptr);
				};
			}
		}

	private:
		HandleId m_element = INVALID_HANDLE;
		IResourceManager* m_resourceManager = nullptr;
	};
	// NOLINTEND
}
