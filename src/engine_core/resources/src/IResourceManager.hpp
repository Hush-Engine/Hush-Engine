/*! \file IResourceManager.hpp
	\author Kyn21kx
	\date 2025-06-28
	\brief 
*/

#pragma once

#include <atomic>
#include <cstdint>

namespace Hush
{
	using HandleId = uint64_t;
	using Deleter = void(*)(void*);

		constexpr HandleId INVALID_HANDLE = 0U;
	
	struct RefCounted {
		void* element = nullptr;
		Deleter deleter = nullptr;
		std::atomic<size_t> count = 0; // We initialize at 0 but IncreaseRefCount will always create it at 1
	};
	
	class IResourceManager {
	public:
		IResourceManager() = default;
		IResourceManager(const IResourceManager &) = default;
		IResourceManager(IResourceManager &&) = delete;
		IResourceManager &operator=(const IResourceManager &) = default;
		IResourceManager &operator=(IResourceManager &&) = delete;
		virtual ~IResourceManager() = default;
		
		virtual RefCounted* IncreaseRefCount(const HandleId& handle) = 0;
		
		virtual void DecreaseRefCount(const HandleId& handle) = 0;
		
		virtual const RefCounted& GetRefCount(const HandleId& handle) = 0;
		
	};
}
