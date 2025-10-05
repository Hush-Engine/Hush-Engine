#pragma once

#include "imgui/imgui.h"
#include <cstdint>
#include <cstring>
#include <string_view>

// Some global types for convinience
namespace Hush {
	
	struct ToastNotification {
		enum class EToastType : std::uint8_t {
			Info,
			Warning,
			Update
		};
	
		static constexpr size_t NOTIFICATION_MAX_LENGTH = 64;
		// NOLINTNEXTLINE
		char text[NOTIFICATION_MAX_LENGTH] = {};

		float remainingTime;

		EToastType type;

		ToastNotification(std::string_view text, float remainingTime, EToastType type) : remainingTime(remainingTime) {
			// NOLINTNEXTLINE
			std::memcpy(this->text, text.data(), text.size());
			this->text[text.size()] = '\0';
			this->type = type;
		}
	};
}

namespace Hush::UIUtils {
	inline bool IsMouseInScene() {
		return !ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	}

	
}
