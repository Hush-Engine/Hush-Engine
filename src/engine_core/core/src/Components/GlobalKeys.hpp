#pragma once

/// This file provides constants for accessing engine resources stored in entities with a predefined key
#include <string_view>

// We append __ at the beginning to avoid conflicts with user side keys

constexpr std::string_view ENGINE_MANAGER = "__HushEngineManager";

constexpr std::string_view EDITOR_CAMERA = "__HushEditorCamera";
