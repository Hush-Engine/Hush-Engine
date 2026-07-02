/*! \file ContentPanel.hpp
	\author Kyn21kx
	\date 2024-05-26
	\brief Provides the UI for the content browser panel
*/

#pragma once
#include "Entity.hpp"
#include "FileMetadata.hpp"
#include "IEditorPanel.hpp"
#include "IFile.hpp"
#include "Ref.hpp"
#include "ResourceManager.hpp"
#include "Shared/ImageTexture.hpp"
#include "VirtualFilesystem.hpp"

namespace Hush
{
	class ContentPanel final : public IEditorPanel
	{
		void Init(Scene *activeScene) noexcept override;

		void OnRender(float deltaTime) override;

	private:
		void GenerateMetaFiles();

		void RefreshDirectory();

		void DrawFiles(bool isMouseInScene);

		void CreateInnerResources(const FileInfo &fileData, const FileMetadata &metadata);

		void MakeMetaFile(const FileInfo &fileData, const FileMetadata &metadata);

		[[nodiscard]]
		bool CanBeDroppedToScene(const FileInfo &fileData) const;

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
		bool m_dirty = true;
	};
} // namespace Hush
