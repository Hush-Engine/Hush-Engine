/*! \file ResourceManager.hpp
	\author Kyn21kx
	\date 2025-06-27
	\brief A resource manager for the editor, not a part of the engine core because we need to know the type of each resource and that introduces dependencies
*/

#pragma once

#include "IResourceManager.hpp"
#include "Shared/ImageTexture.hpp"
#include "Ref.hpp"
#include "Shared/Mesh.hpp"
#include "VirtualFilesystem.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Hush
{
	class ResourceManager final : public IResourceManager {
	public:
		ResourceManager() = default;
		ResourceManager(const ResourceManager &) = delete;
		
	    ResourceManager(ResourceManager&& other) noexcept
	      : m_references(std::move(other.m_references))
	      , m_deletionQueue(std::move(other.m_deletionQueue))
	      , m_loadedResources(std::move(other.m_loadedResources))
	      , m_filesystem(other.m_filesystem)
	    {
	    }
		ResourceManager &operator=(const ResourceManager &) = delete;
		ResourceManager &operator=(ResourceManager &&) = delete;

		void Init(VirtualFilesystem* filesystem);

		RefCounted* IncreaseRefCount(const HandleId& handle) override;
		
		void DecreaseRefCount(const HandleId& handle) override;
		
		const RefCounted& GetRefCount(const HandleId& handle) override;
		
		Ref<ImageTexture> LoadTexture(const std::string_view& path);
		
		Ref<ImageTexture> LoadTexture(const std::string_view& name, const std::byte* data, const size_t& size);

		Ref<Mesh> LoadMesh(const std::string_view& path);

	private:
		std::unordered_map<HandleId, RefCounted> m_references;
		std::vector<HandleId> m_deletionQueue;
		std::unordered_map<uint64_t, HandleId> m_loadedResources;

		VirtualFilesystem* m_filesystem = nullptr;

	};
}
