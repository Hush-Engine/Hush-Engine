/*! \file ResourceManager.hpp
	\author Kyn21kx
	\date 2025-06-27
	\brief A resource manager for the editor, not a part of the engine core because we need to know the type of each
   resource and that introduces dependencies
*/

#pragma once

#include "Components/TextureComponent.hpp"
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
	class ResourceManager final : public IResourceManager
	{
	public:
		enum class EError
		{
			None = 0,
			FileNotFound,
			InvalidData,
			UnsupportedFormat,
			LoadFailed,
			UnknownError
		};

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

		// Ref<ImageTexture> LoadTexture(const std::string_view &path);

		// Ref<ImageTexture> LoadTexture(const std::string_view &name, const std::byte *data, const size_t &size);

		/// @brief Loads a texture from the given path. The path is relative to the virtual filesystem root.
		///
		/// If the texture has already been loaded, returns a reference to the existing texture.
		/// Otherwise, loads the texture from the filesystem, stores it in the resource manager,
		/// and returns a reference to the new texture.
		///
		/// The Ref<TextureComponent> returned by this function *does not* have the texture data uploaded to the GPU
		/// yet. To do the actual GPU upload, the @ref ResourceUploadSystem must be run, which will process all pending
		/// texture uploads and create the corresponding GPU resources. Until then, the TextureComponent will hold a
		/// pointer to a "placeholder". This allows the caller to reference the texture immediately after loading, even
		/// if the GPU upload is deferred to a later stage.
		///
		/// You can check if the texture has been uploaded to the GPU by checking if the TextureComponent's IsLoaded()
		/// method returns true.
		///
		/// As part of this member function, another Ref<> will be created, but it will store the
		///
		/// @param path The path to the texture file, relative to the virtual filesystem root.
		/// @param unloadStrategy Whether to keep the CPU image resident after GPU upload.
		/// @param maxSize If non-zero, the decoded RGBA8 image is box-downscaled so its longest
		///        side is at most this many pixels (e.g. for thumbnails). Textures loaded with a
		///        given maxSize are cached separately from the full-resolution load of the same path.
		///
		/// @return A reference to the loaded texture, or an error if the texture could not be loaded.
		Result<Ref<TextureComponent>, EError> LoadTexture(std::string_view path,
														  TextureComponent::ECpuUnloadStrategy unloadStrategy =
															  TextureComponent::ECpuUnloadStrategy::UnloadAfterUpload,
														  uint32_t maxSize = 0);

		/// @brief Loads a texture from the given raw data.
		///
		/// If a texture with the same name has already been loaded, returns a reference to the existing texture.
		/// Otherwise, creates a new texture from the raw data, stores it in the resource manager, and returns a
		/// reference to the new texture.
		///
		/// @param name The unique name for the texture. This is used to identify the texture in the resource manager.
		/// @param data The raw texture data. It is expected to be in a format that the graphics device can consume
		/// directly (e.g. RGBA8 pixel data).
		///             The resource manager does not perform any decoding or format conversion on the data.
		///
		/// @return A reference to the loaded texture, or an error if the texture could not be created.
		Result<Ref<TextureComponent>, EError> LoadTextureFromData(std::string_view name,
																  std::span<const std::byte> data);

		void FreePending() override;

		Ref<Mesh> LoadMesh(const std::string_view &path);


		template <class T, class... Args>
		Ref<T> AllocateRefKnwonID(uint64_t id, Args &&...args)
		{
			const auto &iterator = this->m_loadedResources.find(id);
			if (iterator != this->m_loadedResources.end())
			{
				HandleId handle = iterator->second;
				auto *instance = reinterpret_cast<T *>(handle);
				return {this, instance, id};
			}
			// NOLINTNEXTLINE
			T *instance = new T(std::forward<Args>(args)...);
			const auto handle = reinterpret_cast<HandleId>(instance);
			this->m_loadedResources[id] = handle;
			return {this, instance, id};
		}

		template <class T, class... Args>
		Ref<T> AllocateRef(const std::string_view &identifier, Args &&...args)
		{
			uint64_t hash = Hashing::Fnv1a64(identifier);
			return AllocateRefKnwonID<T>(hash, std::forward<Args>(args)...);
		}

		template <class T>
		[[nodiscard]]
		Ref<T> GetRefOrNull(uint64_t identifier)
		{
			const auto &iterator = this->m_loadedResources.find(identifier);
			if (iterator != this->m_loadedResources.end())
			{
				HandleId handle = iterator->second;
				auto *instance = reinterpret_cast<T *>(handle);
				return {this, instance, identifier};
			}
			return {this, nullptr};
		}

		template <class T>
		[[nodiscard]]
		Ref<T> GetRefOrNull(const std::string_view &identifier) const
		{
			uint64_t hash = Hashing::Fnv1a64(identifier);
			return GetRefOrNull<T>(hash);
		}

	private:
		std::unordered_map<HandleId, RefCounted> m_references;
		std::vector<HandleId> m_deletionQueue;
		std::unordered_map<uint64_t, HandleId> m_loadedResources;

		VirtualFilesystem *m_filesystem = nullptr;
	};
} // namespace Hush
