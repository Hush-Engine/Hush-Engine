/*! \file ResourceManager.hpp
	\author Kyn21kx
	\date 2025-06-27
	\brief A resource manager for the editor, not a part of the engine core because we need to know the type of each
   resource and that introduces dependencies
*/

#pragma once

#include "Assertions.hpp"
#include "IResourceManager.hpp"
#include "Shared/ImageTexture.hpp"
#include "Ref.hpp"
#include "Shared/Mesh.hpp"
#include "VirtualFilesystem.hpp"
#include "crypto/Hashing.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace Hush
{
	class ResourceManager final : public IResourceManager
	{
	public:
		ResourceManager() = default;
		ResourceManager(const ResourceManager &) = delete;

		ResourceManager(ResourceManager &&other) noexcept
			: m_references(std::move(other.m_references)),
			  m_deletionQueue(std::move(other.m_deletionQueue)),
			  m_loadedResources(std::move(other.m_loadedResources)),
			  m_filesystem(other.m_filesystem)
		{
		}
		ResourceManager &operator=(const ResourceManager &) = delete;
		ResourceManager &operator=(ResourceManager &&) = delete;

		void Init(VirtualFilesystem *filesystem);

		RefCounted *IncreaseRefCount(const HandleId &handle) override;

		RefCounted *DecreaseRefCount(const HandleId &handle) override;

		const RefCounted &GetRefCount(const HandleId &handle) override;

		Ref<ImageTexture> LoadTexture(const std::string_view &path);

		Ref<ImageTexture> LoadTexture(const std::string_view &name, const std::byte *data, const size_t &size);

		void FreePending() override;

		Ref<Mesh> LoadMesh(const std::string_view &path);

		template <class T, class ...Args>
		inline Ref<T> AllocateRef(const std::string_view &identifier, Args&&... args)
		{
			uint64_t hash = Hashing::Fnv1a64(identifier);
			const auto &iterator = this->m_loadedResources.find(hash);
			if (iterator != this->m_loadedResources.end())
			{
				HandleId handle = iterator->second;
				auto *texture = reinterpret_cast<T *>(handle);
				return {this, texture};
			}
			Ref<T> result = {this, new T(std::forward<Args>(args)...)};
			this->m_loadedResources[hash] = reinterpret_cast<HandleId>(result.Get());
			return result;
		}

	private:
		std::unordered_map<HandleId, RefCounted> m_references;
		std::vector<HandleId> m_deletionQueue;
		std::unordered_map<uint64_t, HandleId> m_loadedResources;

		VirtualFilesystem *m_filesystem = nullptr;
	};
} // namespace Hush
