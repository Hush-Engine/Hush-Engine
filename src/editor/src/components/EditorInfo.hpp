#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace Hush
{
	enum class EEditorState : uint16_t
	{
		None = 0,
		FreeLook,
		CommandMode,
		JumpMode
	};

	constexpr std::string_view PLAY_TIME_EVENT_KEY = "HUSH_PLAY";

	struct EditorInfo
	{
		EEditorState currentState = EEditorState::None;
		// If we grow to have more booleans we should really just use flags instead
		bool isMouseOnScene = false;
		bool isPlaying = false;
		std::string lastUsedScenePath;
	};
} // namespace Hush
