#include "CommandPanel.hpp"
#include "definitions/KeyCode.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "InputManager.hpp"

void Hush::CommandPanel::OnRender()
{
	this->HandleInput();
	this->TypeCommand();
	ImGuiWindowClass windowClass{};
	windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	ImGui::SetNextWindowClass(&windowClass);
	ImGui::Begin("Command Panel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse);
	ImGui::Text("%s", this->m_panelText.c_str());
	ImGui::End();
}

void Hush::CommandPanel::HandleInput()
{
	if (InputManager::IsKeyDownThisFrame(EKeyCode::ESCAPE))
	{
		this->m_panelText = DEFAULT_CMD_PANEL_TEXT.data();
		this->m_acceptText = false;
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
