/*! \file TitleBarMenuPanel.hpp
	\author Kyn21kx
	\date 2024-05-25
	\brief The main title bar menu for the Hush Engine
*/

#pragma once

#include "IEditorPanel.hpp"

namespace Hush
{
	class TitleBarMenuPanel final : public IEditorPanel
	{
	public:
		
		void Init(Scene* activeScene) noexcept override;
		
		void OnRender() noexcept override;

	private:
		void FileMenuOptions();
	};
} // namespace Hush
