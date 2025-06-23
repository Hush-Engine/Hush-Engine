#pragma once
#include <cstdint>

enum class EEditorState : uint16_t {
	None = 0,
	FreeLook,
	CommandMode,
	JumpMode
};

struct EditorInfo {
	EEditorState currentState = EEditorState::None;
};

