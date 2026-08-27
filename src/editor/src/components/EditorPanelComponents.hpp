/*! \file EditorPanelComponents.hpp
	\author Leonidas Gonzalez
	\date 2026-05-17
	\brief Shared data defintions across the editor panels and their systems
*/

#pragma once
#include <glm/glm.hpp>

namespace Hush
{
	struct ScenePanelSizeComp
	{
		glm::u32vec2 size{};
	};

	struct GamePanelSizeComp
	{
		glm::u32vec2 size{};
		glm::vec2 position{};  // top-left of content area in window space (for mouse NDC correction)
	};
} // namespace Hush
