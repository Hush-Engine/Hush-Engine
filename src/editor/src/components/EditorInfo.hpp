#pragma once
#include <cstdint>

namespace Hush
{
	enum class EEditorState : uint16_t
	{
		None = 0,
		FreeLook,
		CommandMode,
		JumpMode
	};


	struct EditorInfo
	{
		EEditorState currentState = EEditorState::None;
		// If we grow to have more booleans we should really just use flags instead
		bool isMouseOnScene = false;
	};
} // namespace Hush
