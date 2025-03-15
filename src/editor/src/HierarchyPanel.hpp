/*! \file HierarchyPanel.hpp
	\author Kyn21kx
	\date 2024-05-24
	\brief Panel to display objects loaded to a scene
*/

#pragma once

#include "IEditorPanel.hpp"

namespace Hush
{
	class HierarchyPanel final : public IEditorPanel
	{
	public:
		void Init(Scene *activeScene) noexcept override;

		void OnRender() override;

	private:
		Scene *m_activeScene;
	};
} // namespace Hush
