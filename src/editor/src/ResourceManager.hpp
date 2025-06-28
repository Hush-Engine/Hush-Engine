/*! \file ResourceManager.hpp
	\author Kyn21kx
	\date 2025-06-27
	\brief A resource manager for the editor, not a part of the engine core because we need to know the type of each resource and that introduces dependencies
*/

#pragma once

#include "IResourceManager.hpp"
#include <unordered_map>

namespace Hush
{
	class ResourceManager final : public IResourceManager {
	public:
		ResourceManager(const ResourceManager &) = default;
		ResourceManager(ResourceManager &&) = delete;
		ResourceManager &operator=(const ResourceManager &) = default;
		ResourceManager &operator=(ResourceManager &&) = delete;

		void IncreaseRefCount(const HandleId& handle) override;
		
		void DecreaseRefCount(const HandleId& handle) override;
		
		const RefCounted& GetRefCount(const HandleId& handle) override;
		
	private:
		std::unordered_map<HandleId, RefCounted> m_references;
	};
}
