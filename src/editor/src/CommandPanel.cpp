#include "CommandPanel.hpp"
#include "Assertions.hpp"
#include "Entity.hpp"
#include "InspectorPanel.hpp"
#include "Scene.hpp"
#include "definitions/KeyCode.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "InputManager.hpp"
#include <array>
#include <cstdint>
#include <cstring>
#include <magic_enum/magic_enum.hpp>
#include <string_view>
#include "UI.hpp"
#include "MathUtils.hpp"
#include "StringUtils.hpp"
#include "Components/Transform.hpp"
#include <zadeh/StringArrayFilterer.h>
#include <zadeh/zadeh.h>

constexpr std::array<std::string_view, 4> BUILT_IN_COMMANDS = {"add-entity", "find-entity", "add-component", "help"};

HUSH_STATIC_ASSERT(BUILT_IN_COMMANDS.size() == magic_enum::enum_count<Hush::CommandPanel::EBuiltinCommands>(),
				   "Built-in commands enum does not match with array");

void Hush::CommandPanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;
}

void Hush::CommandPanel::OnRender()
{
	ImGuiWindowClass windowClass{};
	windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	this->HandleInput();
	this->TypeCommand();
	this->UpdateCommandList();
	if (this->m_currState == EState::SearchMode)
	{
		this->FindEntityPopup();
	}
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
		this->m_currState = EState::Editing;
		this->m_panelText = "";
	}
}

void Hush::CommandPanel::TypeCommand()
{
	if (this->m_currState != EState::Editing)
	{
		return;
	}
	char currentInput = 0;
	if (InputManager::FetchCharThisFrame(&currentInput))
	{
		this->m_currState = EState::Editing;
		this->m_panelText += currentInput;
		return;
	}

	bool shouldDelete = ImGui::IsKeyPressed(ImGuiKey_Backspace, true) && this->m_panelText.size() > 1;
	if (shouldDelete)
	{
		size_t lastCharacterIdx = this->m_panelText.size() - 1;
		this->m_panelText = this->m_panelText.substr(0, lastCharacterIdx);
	}
}

void Hush::CommandPanel::CloseCommandMode()
{
	this->m_currState = EState::None;
	this->m_panelText = DEFAULT_CMD_PANEL_TEXT.data();
	this->m_selectedCommandIdx = -1;
	this->m_keyboardFocusSet = false;
}

void Hush::CommandPanel::SubmitCommand(EBuiltinCommands command, std::string_view textCmd)
{
	Entity::EntityId entityToCreate = 0;
	switch (command)
	{
	case EBuiltinCommands::AddEntity:
		if (textCmd.empty())
		{
			// Open the entity search panel or create a new one
			break;
		}
		// Interpret the rest of the text command as the name of the entity to add
		entityToCreate = this->m_activeScene->CreateEntityWithName(textCmd).GetId();
		this->m_activeScene->RegisterComponentId(textCmd, entityToCreate);
		this->m_activeScene->EntityFromId(entityToCreate)->AddComponent<Transform>();
		UI::Get().GetPanel<InspectorPanel>().SetInspectTarget(entityToCreate);
		break;
	case EBuiltinCommands::FindEntity:
		this->m_currState = EState::SearchMode;
		this->m_keyboardFocusSet = true;
		return;
	case EBuiltinCommands::AddComponent:
	case EBuiltinCommands::Help:
		break;
	}
	this->CloseCommandMode();
}

void Hush::CommandPanel::UpdateCommandList()
{
	if (this->m_currState != EState::Editing)
	{
		return;
	}

	EState previousState = this->m_currState;
	if (ImGui::IsKeyPressed(ImGuiKey_Tab, true))
	{
		this->m_currState = EState::ForceFocus;
		int32_t nextIdx = this->m_selectedCommandIdx + 1;
		this->m_selectedCommandIdx = MathUtils::CircleBack(nextIdx, 0, BUILT_IN_COMMANDS.size() - 1);
	}

	ImGui::SetNextWindowSize({this->m_commandPanelWidth, this->m_commandPanelHeight * 2.0F});

	ImGui::SetNextWindowPos(
		{this->m_commandPanelPos.x, this->m_commandPanelPos.y - (this->m_commandPanelHeight * 2.0F)});
	ImGui::SetNextWindowBgAlpha(0.5F);
	ImGui::Begin("Available commands", nullptr, ImGuiWindowFlags_NoCollapse);
	// Show all commands that match
	ImDrawList *drawList = ImGui::GetWindowDrawList();
	for (size_t i = 0; i < BUILT_IN_COMMANDS.size(); i++)
	{
		const std::string_view &command = BUILT_IN_COMMANDS.at(i);
		bool hovered = false;
		bool forceHover = this->m_selectedCommandIdx == i;
		bool submitted = UI::CustomSelectable(command.data(), &hovered, drawList, forceHover);

		if (hovered && this->m_currState == EState::ForceFocus)
		{
			this->m_panelText = std::string(":") + command.data();
		}
		if (submitted)
		{
			// Next words from space
			auto offset = static_cast<int32_t>(this->m_panelText.find(' ')) + 1;
			this->SubmitCommand(static_cast<EBuiltinCommands>(this->m_selectedCommandIdx),
								StringUtils::SubstrView(this->m_panelText, offset, this->m_panelText.size()));
		}
	}
	this->m_currState = this->m_currState != EState::None && this->m_currState != EState::SearchMode
							? previousState
							: this->m_currState;
	ImGui::End();
}

void Hush::CommandPanel::FindEntityPopup()
{
	ImGui::SetNextWindowBgAlpha(0.5F);
	ImGui::Begin("Entity search");
	ImGui::Text("Search for an entity");
	if (this->m_keyboardFocusSet) {
		ImGui::SetKeyboardFocusHere();
		memset(this->m_searchEntityName, 0, MAX_ALLOWED_ENTITY_NAME);
		this->m_keyboardFocusSet = false;
	}
	// If we type, we set the focus
	char _ = '0';
	if (InputManager::FetchCharThisFrame(&_)) {
		ImGui::SetKeyboardFocusHere();
	}
	ImGui::InputTextWithHint("##Search", "i.e. Player", this->m_searchEntityName, MAX_ALLOWED_ENTITY_NAME);
	// Then find all entities in the scene here
	Query<Transform> query = this->m_activeScene->CreateQuery<Transform>();
	std::vector<std::string> entityNames;
	std::string_view searchEntityName(this->m_searchEntityName);
	entityNames.reserve(query.begin().Size());
	query.Each([&entityNames, &searchEntityName, this](Entity& entity, Transform& transform) {
       	std::string_view currEntityName = entity.GetName().value_or("");
    	if (searchEntityName.empty()) {
    		RenderEntitySelectable(currEntityName, entity.GetId());
    	}
       entityNames.emplace_back(currEntityName);
	});
	
	zadeh::StringArrayFilterer<std::vector<std::string>> filterer{};
	filterer.set_candidates(entityNames);

	std::vector<size_t> indices = filterer.filter_indices(this->m_searchEntityName);
	for (size_t idx : indices) {
		RenderEntitySelectable(entityNames.at(idx), query.begin().GetEntityId(idx));
	}

	ImGui::End();
}


void Hush::CommandPanel::RenderEntitySelectable(const std::string_view& entityName, Entity::EntityId entityId) {
	
	if (!ImGui::Selectable(entityName.data())) {
		return;
	}
	
	UI::Get().GetPanel<InspectorPanel>().SetInspectTarget(entityId);
	this->CloseCommandMode();
	
}


