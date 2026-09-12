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

	enum class ESelectedItemType {
		None = 0,
		Entity,
		Component,
		File
	};

	struct SelectedItemInfo {
		ESelectedItemType type = ESelectedItemType::None;
		uint64_t value = 0; // Could be a pointer, could be an ImGui ID, could be an entity id, a file, etc
	};

	constexpr std::string_view PLAY_TIME_EVENT_KEY = "HUSH_PLAY";

	struct EditorInfo
	{
		EEditorState currentState = EEditorState::None;
		// If we grow to have more booleans we should really just use flags instead
		bool isMouseOnScene = false;
		bool isPlaying = false;
		std::string lastUsedScenePath;
		SelectedItemInfo currentSelection = {};
	};
} // namespace Hush
