#include "CommandPanel.hpp"
#include "definitions/KeyCode.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "InputManager.hpp"
#include <array>
#include <cstdint>
#include <string_view>
#include "UI.hpp"
#include "MathUtils.hpp"

constexpr std::array<std::string_view, 3> BUILT_IN_COMMANDS = {"add-entity", "find-entity", "add-component"};

void Hush::CommandPanel::OnRender()
{
	ImGuiWindowClass windowClass{};
	windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	this->HandleInput();
	this->TypeCommand();
	this->UpdateCommandList();
	ImGui::SetNextWindowClass(&windowClass);
	ImGui::Begin("Command Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
	ImVec2 currentsize = ImGui::GetWindowSize();
	this->m_commandPanelPos = ImGui::GetWindowPos();
	this->m_commandPanelWidth = currentsize.x;
	this->m_commandPanelHeight = currentsize.y;
	ImGui::Text("%s", this->m_panelText.c_str());
	ImGui::End();
}

void Hush::CommandPanel::HandleInput()
{
	if (InputManager::IsKeyDownThisFrame(EKeyCode::ESCAPE))
	{
		this->CloseCommandMode();
		return;
	}
	char pressedChar = 0;
	if (InputManager::FetchCharThisFrame(&pressedChar) && pressedChar == ':')
	{
		this->m_acceptText = true;
		this->m_panelText = "";
	}
}

void Hush::CommandPanel::TypeCommand()
{
	if (!this->m_acceptText)
	{
		return;
	}
	char currentInput = 0;
	if (InputManager::FetchCharThisFrame(&currentInput))
	{
		this->m_panelText += currentInput;
		return;
	}
	float timeSinceLastInput = this->m_inputTimer.Ellapsed();
	constexpr float millisToAcceptBackspace = 100;
	bool shouldDelete = InputManager::IsKeyDown(EKeyCode::BACKSPACE) && timeSinceLastInput >= millisToAcceptBackspace && this->m_panelText.size() > 1;
	if (shouldDelete)
	{
		this->m_inputTimer.Reset();
		size_t lastCharacterIdx = this->m_panelText.size() - 1;
		this->m_panelText = this->m_panelText.substr(0, lastCharacterIdx);
	}
}


void Hush::CommandPanel::CloseCommandMode() {
	this->m_panelText = DEFAULT_CMD_PANEL_TEXT.data();
	this->m_acceptText = false;
	this->m_selectedCommandIdx = -1;
}

void Hush::CommandPanel::UpdateCommandList() {
	if (!this->m_acceptText) {
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_Tab, true)) {
		if (ImGui::IsKeyPressed(ImGuiKey_ModShift, true)) {
			
		}
		this->m_selectedCommandIdx = MathUtils::CircleBack(this->m_selectedCommandIdx + 1, 0, BUILT_IN_COMMANDS.size() - 1);
	}

		
	ImGuiIO& imGuiIO = ImGui::GetIO();
	ImGui::SetNextWindowSize({this->m_commandPanelWidth, this->m_commandPanelHeight * 4.0F});
	// ImGui::SetNextWindowPos({ imGuiIO.DisplaySize.x / 2, imGuiIO.DisplaySize.y / 2 });
	ImGui::SetNextWindowPos({ this->m_commandPanelPos.x, imGuiIO.DisplaySize.y / 2 });
	ImGui::SetNextWindowBgAlpha(0.3F);
	ImGui::Begin("Available commands", nullptr, ImGuiWindowFlags_NoCollapse);
	// Show all commands that match
	ImDrawList* drawList = ImGui::GetWindowDrawList();	
	for(size_t i = 0; i < BUILT_IN_COMMANDS.size(); i++) {
		const std::string_view& command = BUILT_IN_COMMANDS.at(i);
		bool hovered = false;
		bool forceHover = this->m_selectedCommandIdx == i;
		UI::CustomSelectable(command.data(), &hovered, drawList, forceHover);
        if (hovered) {
            this->m_panelText = std::string(":") + command.data();
        }        
    }
    ImGui::End();
}

