/*! \file ContentPanel.hpp
	\author Kyn21kx
	\date 2024-05-26
	\brief Provides the UI for the content browser panel
*/

#pragma once
#include "Entity.hpp"
#include "IEditorPanel.hpp"
#include "IFile.hpp"
#include "Ref.hpp"
#include "ResourceManager.hpp"
#include "Shared/ImageTexture.hpp"
#include "VirtualFilesystem.hpp"

#include <filesystem>
#include <string>
#include <unordered_map>

namespace Hush
{
	class ContentPanel final : public IEditorPanel
	{
		void Init(Scene *activeScene) noexcept override;

		void OnRender(float deltaTime) override;

	private:

		/// @brief Utility struct to pass in virtual file paths and metadata, not relying on FileInfo itself
		struct DroppableFile
		{
			std::string virtualPath;
			const FileInfo* fileInfo;
		};

		void RefreshDirectory();

		/// Marks the panel dirty (triggering a re-list) when the content directory's
		/// modification time changes — i.e. an asset was added, removed or renamed
		/// outside the editor.
		void MarkDirtyIfContentChanged();

		void DrawFiles(bool isMouseInScene);

		/// Load (cooked-first, via the VFS) and GPU-upload a thumbnail for an image
		/// asset, caching it by virtual path. Returns the TextureComponent once it is
		/// GPU-ready, or nullptr while the async upload is still in flight / on failure.
		[[nodiscard]]
		TextureComponent *ResolveThumbnail(const std::string &vpath);

		[[nodiscard]]
		static bool IsImageExtension(EFileExtension ext);

		[[nodiscard]]
		bool CanBeDroppedToScene(const DroppableFile &fileData) const;

		ResourceManager *m_resourceManager = nullptr;
		VirtualFilesystem *m_filesystem = nullptr;
		Scene *m_scene = nullptr;
		// TEMP: <a href="https://www.flaticon.com/free-icons/folder" title="folder icons">Folder icons created by Gajah
		// Mada - Flaticon</a>
		Ref<ImageTexture> m_folderImage;
		Ref<ImageTexture> m_fileImage;

		std::vector<FileInfo> m_currentItems;
		std::string m_currentWorkingDirectory = "res://";
		ComponentRef m_editorInfoRef{};
		DroppableFile m_currentDroppable; // This is needed to keep the string intact
		bool m_dirty = true;

		/// Last-seen modification time of the content directory, used to detect
		/// external add/remove/rename of assets and refresh the listing.
		std::filesystem::file_time_type m_lastContentWriteTime{};

		/// Image-asset thumbnails, keyed by virtual path. A null Ref caches a failed load
		/// so it isn't retried every frame. The backing Ref keeps the texture alive.
		std::unordered_map<std::string, Ref<TextureComponent>> m_thumbnailCache;

		/// Holder entities that carry each thumbnail's Ref<TextureComponent> so the
		/// ResourceUploadSystem observer picks them up for GPU upload. Each holder is
		/// destroyed once its upload completes, so the scene doesn't accumulate one
		/// entity per displayed thumbnail.
		std::unordered_map<std::string, Entity> m_thumbnailHolders;
	};
} // namespace Hush
