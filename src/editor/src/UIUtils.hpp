#pragma once

#include "imgui/imgui.h"
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>

// Some global types for convenience
namespace Hush {
	
	struct ToastNotification {
		enum class EToastType : std::uint8_t {
			Info,
			Warning,
			Update
		};
	
		static constexpr size_t NOTIFICATION_MAX_LENGTH = 64;
		std::string text;

		float remainingTime;
		float originalTime;

		EToastType type;

		ToastNotification(std::string_view text, float remainingTime, EToastType type) {
			// NOLINTBEGIN
			this->text = text;
			this->remainingTime = remainingTime;
			this->originalTime = remainingTime;
			this->type = type;
			// NOLINTEND
		}
	};
}

namespace Hush::UIUtils {
	inline bool IsMouseInScene() {
		return !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	}

	
}
