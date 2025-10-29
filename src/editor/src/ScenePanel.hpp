/*! \file ScenePanel.hpp
	\author Leonidas Gonzalez
	\date 2024-05-27
	\brief ImGui Panel where the Scene is rendered
*/

#pragma once

#include "IEditorPanel.hpp"

namespace Hush
{
	class ScenePanel final : public IEditorPanel
	{
	public:
		void Init(Scene *activeScene) noexcept override;
		void OnRender(float deltaTime) noexcept override;
	};
} // namespace Hush
