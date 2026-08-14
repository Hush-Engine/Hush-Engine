#include "NotificationPanel.hpp"
#include "Logger.hpp"
#include "MathUtils.hpp"
#include "Shared/Types/Color.hpp"
#include "UIUtils.hpp"
#include "WindowManager.hpp"
#include "WindowRenderer.hpp"
#include "imgui/imgui.h"
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <string_view>

constexpr ImU32 INFO_TOAST_COLOR = IM_COL32(3, 182, 252, 255);
constexpr ImU32 WARN_TOAST_COLOR = Hush::Color::WarnYellow().ToColor32ABGR();
constexpr ImU32 ERROR_TOAST_COLOR = Hush::Color::Red().ToColor32ABGR();
constexpr ImU32 UPDATE_TOAST_COLOR = IM_COL32(3, 182, 252, 255);
constexpr ImGuiWindowFlags_ TOAST_FLAGS = ImGuiWindowFlags_NoScrollbar;

void Hush::NotificationPanel::OnRender(float deltaTime)
{
	// Get all entries in the queue and fade them out
	int32_t currentToastIndex = -1;
	this->m_toastQuery.Each([&currentToastIndex, this, deltaTime](Entity &entity, ToastNotification &notification) {
		currentToastIndex++; // Increase this first for early return
		if (notification.remainingTime <= 0.f)
		{
			this->m_scene->DestroyEntity(entity);
			return;
		}
		// NOLINTBEGIN

		std::string_view name = magic_enum::enum_name(notification.type);

		switch (notification.type)
		{
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
		case ToastNotification::EToastType::Error:
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ERROR_TOAST_COLOR);
			ImGui::PushStyleColor(ImGuiCol_TitleBg, ERROR_TOAST_COLOR);
			ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ERROR_TOAST_COLOR);
			break;
		}

		notification.remainingTime -= deltaTime;
		const auto renderTargetSize = WindowManager::GetMainWindow()->GetWindowSize();
		constexpr ImVec2 maxDimensions = {400, 100}; // Some way to make height auto grow??
		constexpr float padding = 8.f;
		float wrapWidth =
			maxDimensions.x -
			padding * 2; // 2 is for the window padding (padding for our component + padding for the window)
		ImVec2 textSize = ImGui::CalcTextSize(notification.text.c_str(), nullptr, false, wrapWidth);

		float toastWindowHeight = textSize.y + ImGui::GetFrameHeightWithSpacing() + padding * 2;

		constexpr float minHeight = 100.0f;
		constexpr float maxHeight = 200.0f;
		toastWindowHeight = MathUtils::Clamp(toastWindowHeight, minHeight, maxHeight);

		ImVec2 toastSize = {maxDimensions.x, toastWindowHeight};
		// constexpr float maxPercentageHeightScreen = 0.85f;
		// We need something different, the next toast notification will be at the currToastIndex * the max height
		constexpr float verticalGap = 4.f;
		// constexpr float startYOffset = 20;
		const ImVec2 toastPos =
			ImVec2(renderTargetSize.x - maxDimensions.x,
				   renderTargetSize.y - minHeight - (currentToastIndex * (toastSize.y + verticalGap)));
		ImGui::SetNextWindowPos(toastPos);
		ImGui::SetNextWindowSize(toastSize);
		std::string notificationName = name.data();
		// Craft unique ID
		notificationName.append("##").append(std::to_string(entity.GetId()));
		ImGui::Begin(notificationName.c_str(), nullptr, TOAST_FLAGS);
		ImGui::TextWrapped("%s", notification.text.c_str());

		// NOLINTEND
		ImGui::PopStyleColor(3);
		ImGui::End();
	});
}

void Hush::NotificationPanel::Init(Scene *activeScene) noexcept
{
	this->m_scene = activeScene;
	this->m_toastQuery = activeScene->CreateQuery<ToastNotification>();
}
