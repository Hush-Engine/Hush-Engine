#include "NotificationPanel.hpp"
#include "imgui/imgui.h"
#include <magic_enum/magic_enum.hpp>
#include <string_view>

constexpr ImU32 INFO_TOAST_COLOR = IM_COL32(3, 182, 252, 127);
constexpr ImU32 WARN_TOAST_COLOR = IM_COL32(3, 182, 252, 127);
constexpr ImU32 UPDATE_TOAST_COLOR = IM_COL32(3, 182, 252, 127);

void Hush::NotificationPanel::OnRender() {
	// Get all entries in the queue and fade them out
	this->m_toastQuery.Each([](ToastNotification& notification) {
		// Queries will fetch the newer entities first?
		// Show the notification and fade it out
		// NOLINTBEGIN
		
		std::string_view name = magic_enum::enum_name(notification.type);

		switch (notification.type) {
		case ToastNotification::EToastType::Info:
			ImGui::PushStyleColor(ImGuiCol_WindowBg, INFO_TOAST_COLOR);
			ImGui::PushStyleColor(ImGuiCol_TitleBg, INFO_TOAST_COLOR);
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, INFO_TOAST_COLOR);
			break;
		case ToastNotification::EToastType::Warning:
			ImGui::PushStyleColor(ImGuiCol_WindowBg, WARN_TOAST_COLOR);
			ImGui::PushStyleColor(ImGuiCol_TitleBg, WARN_TOAST_COLOR);
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, WARN_TOAST_COLOR);
			break;
		case ToastNotification::EToastType::Update:
			ImGui::PushStyleColor(ImGuiCol_WindowBg, UPDATE_TOAST_COLOR);
			ImGui::PushStyleColor(ImGuiCol_TitleBg, UPDATE_TOAST_COLOR);
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, UPDATE_TOAST_COLOR);
			break;
		}

		ImGui::Begin(name.data());
		ImGui::Text("%s", notification.text);

		// NOLINTEND
		ImGui::PopStyleColor(3);
		ImGui::End();
	});
}

void Hush::NotificationPanel::Init(Scene *activeScene) noexcept {
	this->m_toastQuery = activeScene->CreateQuery<ToastNotification>();
}


