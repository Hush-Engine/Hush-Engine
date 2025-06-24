#include "CommandPanel.hpp"
#include "BitwiseUtils.hpp"
#include "Components/LocalTransform.hpp"
#include "Entity.hpp"
#include "HushEngine.hpp"
#include "InspectorPanel.hpp"
#include "Logger.hpp"
#include "Scene.hpp"
#include "components/EditorInfo.hpp"
#include "crypto/Hashing.hpp"
#include "definitions/KeyCode.hpp"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "InputManager.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <flecs.h>
#include <magic_enum/magic_enum.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "UI.hpp"
#include "MathUtils.hpp"
#include "Components/WorldTransform.hpp"
#include "ArrayUtils.hpp"
#include "StringUtils.hpp"
#include "Shared/DirectionalLight.hpp"

constexpr std::array<std::string_view, 4> BUILT_IN_COMMANDS = {"add-entity", "find-entity", "add-component", "help"};

// NOLINTNEXTLINE
#define CALC_CMD_HASH(idx) Hush::Hashing::Fnv1a(BUILT_IN_COMMANDS[idx].data(), BUILT_IN_COMMANDS[idx].size())

enum class EBuiltinCommands : uint32_t
{
	AddEntity = CALC_CMD_HASH(0),
	FindEntity = CALC_CMD_HASH(1),
	AddComponent = CALC_CMD_HASH(2),
	Help = CALC_CMD_HASH(3)
};

void Hush::CommandPanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;
	this->m_currentlyAvailableCommands = {BUILT_IN_COMMANDS.begin(), BUILT_IN_COMMANDS.end()};
	
	activeScene->CreateQuery<EditorInfo>().Each([this](Entity& entity, EditorInfo& infoRef){
		this->m_editorInfo = &infoRef;
    });
}

void Hush::CommandPanel::OnRender()
{
	ImGuiWindowClass windowClass{};
	windowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoTabBar;
	this->HandleInput();
	this->TypeCommand();
	this->UpdateCommandList();
	switch (this->m_currState)
	{
	case EState::SearchMode:
		this->FindEntityPopup();
		break;
	case EState::AddComponentMode:
		this->AddComponentPopup();
		break;
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

	this->m_editorInfo->currentState = EEditorState::CommandMode;
	char currentInput = 0;
	// When typing the command we should do a pass of available commands to update with fuzzy search
	if (InputManager::FetchCharThisFrame(&currentInput))
	{
		this->m_currState = EState::Editing;
		this->m_panelText += currentInput;
		this->RebuildAvailableCommands();
		return;
	}

	bool shouldDelete = ImGui::IsKeyPressed(ImGuiKey_Backspace, true) && this->m_panelText.size() > 1;
	if (shouldDelete)
	{
		size_t lastCharacterIdx = this->m_panelText.size() - 1;
		this->m_panelText = this->m_panelText.substr(0, lastCharacterIdx);
		this->RebuildAvailableCommands();
	}
}

void Hush::CommandPanel::CloseCommandMode()
{
	this->m_currState = EState::None;
	this->m_panelText = DEFAULT_CMD_PANEL_TEXT.data();
	this->m_selectedCommandIdx = -1;
	this->m_keyboardFocusSet = false;
	if (this->m_editorInfo->currentState == EEditorState::CommandMode) {
		this->m_editorInfo->currentState = EEditorState::None;
	} 
}

void Hush::CommandPanel::SubmitCommand(uint32_t command, const std::string_view &textCmd)
{
	Entity::EntityId entityToCreate = 0;
	switch (static_cast<EBuiltinCommands>(command))
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
		this->m_activeScene->EntityFromId(entityToCreate)->AddComponent<WorldTransform>();
		this->m_activeScene->EntityFromId(entityToCreate)->AddComponent<LocalTransform>();
		UI::Get().GetPanel<InspectorPanel>().SetInspectTarget(entityToCreate);
		break;
	case EBuiltinCommands::FindEntity:
		this->m_currState = EState::SearchMode;
		this->m_keyboardFocusSet = true;
		return;
	case EBuiltinCommands::AddComponent:
		this->m_currState = EState::AddComponentMode;
		this->m_keyboardFocusSet = true;
		return;
	case EBuiltinCommands::Help:
		break;
	default:
		// Show error
		// Then fade out
		LogFormat(ELogLevel::Error, "No command called {} was found", textCmd);
		break;
	}
	this->CloseCommandMode();
}

void Hush::CommandPanel::RebuildAvailableCommands()
{
	// Update only if the panel text.size() > 1 bc it still counts the colon
	if (this->m_panelText.size() < 2)
	{
		// Hard set to the original state
		this->m_currentlyAvailableCommands = {BUILT_IN_COMMANDS.begin(), BUILT_IN_COMMANDS.end()};
		return;
	}
	// Reconstructing the entire vector is still cheaper than checking for existing instances of a match
	this->m_currentlyAvailableCommands.clear();
	using Arr_t = std::array<std::string_view, BUILT_IN_COMMANDS.size()>;
	// The query string is a substring on start offset 1, and wherever we find a space or nPos
	const size_t endIdx = this->m_panelText.find(' ');
	const std::string queryStr = this->m_panelText.substr(1, endIdx);

	this->m_currentlyAvailableCommands = ArrayUtils::FuzzyFind<Arr_t, std::string_view>(BUILT_IN_COMMANDS, queryStr);
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
	for (size_t i = 0; i < this->m_currentlyAvailableCommands.size(); i++)
	{

		const std::string_view &command = this->m_currentlyAvailableCommands.at(i);
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
			std::string_view cmdText;
			if (offset != 0)
			{
				// The command was submitted with additional data
				cmdText = StringUtils::SubstrView(this->m_panelText, offset, (int32_t)this->m_panelText.size());
			}
			std::string pureCommand = this->m_panelText.substr(1, offset - 2);
			uint32_t commandHash = Hashing::Fnv1a(pureCommand.data(), pureCommand.size());
			this->SubmitCommand(commandHash, cmdText);
		}
	}
	this->m_currState = this->m_currState != EState::None &&
								!Bitwise::HasCompositeFlag((int32_t)this->m_currState, (int32_t)EState::IsPopupMode)
							? previousState
							: this->m_currState;
	ImGui::End();
}

void Hush::CommandPanel::AddComponentPopup()
{
	// If we don't have an entity selected in the inspector we should first find one
	std::optional<Entity> &inspectTarget = UI::Get().GetPanel<InspectorPanel>().GetInspectTarget();
	if (!inspectTarget.has_value())
	{
		this->FindEntityPopup("No selected entity in the inspector, please select one...");
		return;
	}
	UI::BeginCenterPopup("Add Component", true);
	ImGui::Text("Select a component to add");
	if (this->m_keyboardFocusSet)
	{
		ImGui::SetKeyboardFocusHere();
		memset(this->m_searchInputText, 0, MAX_ALLOWED_ENTITY_NAME);
		this->m_keyboardFocusSet = false;
	}
	UI::InputTextWithHint("##Search", "i.e. Rigidbody", this->m_searchInputText, MAX_ALLOWED_ENTITY_NAME, true);
	// Find built in components
	using Arr_t = std::array<std::string_view, 2>;
	constexpr Arr_t builtinComponents = {"Transform", "DirectionalLight"};
	std::vector<std::string_view> componentNames =
		ArrayUtils::FuzzyFind<Arr_t, std::string_view>(builtinComponents, this->m_searchInputText);
	for (const std::string_view &componentName : componentNames)
	{
		if (!ImGui::Selectable(componentName.data()))
		{
			continue;
		}
		LogFormat(ELogLevel::Info, "Selected {}", componentName);
		// Add the component to the currently selected entity
		switch (Hashing::Fnv1a(componentName))
		{
		case Hashing::Fnv1a("Transform"):
			inspectTarget.value().AddComponent<WorldTransform>();
			break;
		case Hashing::Fnv1a("DirectionalLight"):
			inspectTarget.value().AddComponent<DirectionalLight>();
			break;
		}
		this->CloseCommandMode();
	}

	ImGui::End();
}

void Hush::CommandPanel::FindEntityPopup(const char *overrideLabel)
{
	const char *label = overrideLabel != nullptr ? overrideLabel : "Entity search";
	UI::BeginCenterPopup(label, true);
	ImGui::Text("Search for an entity");
	if (this->m_keyboardFocusSet)
	{
		ImGui::SetKeyboardFocusHere();
		memset(this->m_searchInputText, 0, MAX_ALLOWED_ENTITY_NAME);
		this->m_keyboardFocusSet = false;
	}
	UI::InputTextWithHint("##Search", "i.e. Player", this->m_searchInputText, MAX_ALLOWED_ENTITY_NAME, true);
	// Then find all entities in the scene here
	Query<WorldTransform> query = this->m_activeScene->CreateQuery<WorldTransform>();
	std::vector<std::string> entityNames;
	std::string_view searchEntityName(this->m_searchInputText);
	entityNames.reserve(query.begin().Size());
	query.Each([&entityNames, &searchEntityName, this](Entity &entity, WorldTransform &transform) {
		std::string_view currEntityName = entity.GetName().value_or("");
		if (searchEntityName.empty())
		{
			RenderEntitySelectable(currEntityName, entity.GetId());
		}
		entityNames.emplace_back(currEntityName);
	});

	std::vector<size_t> indices =
		ArrayUtils::FuzzyFindIndices<std::vector<std::string>, std::string>(entityNames, this->m_searchInputText);

	for (size_t idx : indices)
	{
		RenderEntitySelectable(entityNames.at(idx), query.begin().GetEntityId(idx));
	}

	ImGui::End();
}

void Hush::CommandPanel::RenderEntitySelectable(const std::string_view &entityName, Entity::EntityId entityId)
{

	if (!ImGui::Selectable(entityName.data()))
	{
		return;
	}

	UI::Get().GetPanel<InspectorPanel>().SetInspectTarget(entityId);
	this->CloseCommandMode();
}
