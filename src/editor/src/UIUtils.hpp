#pragma once

#include "imgui/imgui.h"

namespace UIUtils {
	inline bool IsMouseInScene() {
		return !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	}
}
