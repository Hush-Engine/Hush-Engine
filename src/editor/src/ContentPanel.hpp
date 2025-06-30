/*! \file ContentPanel.hpp
	\author Kyn21kx
	\date 2024-05-26
	\brief Provides the UI for the content browser panel
*/

#pragma once
#include "IEditorPanel.hpp"
#include "Ref.hpp"
#include "ResourceManager.hpp"
#include "Shared/ImageTexture.hpp"
#include "VirtualFilesystem.hpp"

namespace Hush
{
	class ContentPanel final : public IEditorPanel
	{
		void Init(Scene *activeScene) noexcept override;

		void OnRender() override;

	private:
		void RefreshDirectory();
		
		ResourceManager* m_resourceManager;
		VirtualFilesystem* m_filesystem;
		// TEMP: <a href="https://www.flaticon.com/free-icons/folder" title="folder icons">Folder icons created by Gajah Mada - Flaticon</a>
		Ref<ImageTexture> m_folderImage;
		Ref<ImageTexture> m_fileImage;

		std::vector<std::string> m_currentItems;
		std::string m_currentWorkingDirectory = "res://";
		bool m_dirty = true;
	};
} // namespace Hush
