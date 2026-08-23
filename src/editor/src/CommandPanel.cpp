#include "CommandPanel.hpp"
#include "Assertions.hpp"
#include "BitwiseUtils.hpp"
#include "Components/LocalTransform.hpp"
#include "Entity.hpp"
#include "HushEngine.hpp"
#include "InspectorPanel.hpp"
#include "Logger.hpp"
#include "Scene.hpp"
#include "ScenePanel.hpp"
#include "ScriptingHost.hpp"
#include "UIUtils.hpp"
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
#include <glm/ext/vector_uint2_sized.hpp>
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
#include "SystemSelection.hpp"

constexpr std::array<std::string_view, 5> BUILT_IN_COMMANDS = {"add-entity", "find-entity", "add-component",
															   "add-system", "help"};

// NOLINTNEXTLINE
#define CALC_CMD_HASH(idx)                                                                                             \
	Hush::Hashing::Fnv1a(BUILT_IN_COMMANDS[idx].data(), static_cast<uint32_t>(BUILT_IN_COMMANDS[idx].size()))

enum class EBuiltinCommands : uint32_t
{
	AddEntity = CALC_CMD_HASH(0),
	FindEntity = CALC_CMD_HASH(1),
	AddComponent = CALC_CMD_HASH(2),
	AddSystem = CALC_CMD_HASH(3),
	Help = CALC_CMD_HASH(4)
};

void Hush::CommandPanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;
	this->m_currentlyAvailableCommands = {BUILT_IN_COMMANDS.begin(), BUILT_IN_COMMANDS.end()};

	activeScene->CreateQuery<EditorInfo, ScriptingHost>().Each([this]([[maybe_unused]]
																	  Entity &entity,
																	  EditorInfo &infoRef,
																	  ScriptingHost &scriptingHostRef) {
		this->m_editorInfo = &infoRef;
		this->m_scriptingHost = &scriptingHostRef;
	});
}

void Hush::CommandPanel::OnRender([[maybe_unused]] float deltaTime)
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
	case EState::AddSystemMode: {
		bool popupOpen =
			SystemSelection::RenderSystemListWindow(this->m_activeScene, this->m_scriptingHost, &this->m_selectedItem);
		if (!popupOpen)
		{
			this->CloseCommandMode();
		}
		break;
	}
	case EState::None:
	case EState::Editing:
	case EState::ForceFocus:
	case EState::IsPopupMode:
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
	// FIX: FetchCharThisFrame from InputManager was not working, switched to the ImGui one
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
	this->m_selectedItem = -1;
	this->m_keyboardFocusSet = false;
	if (this->m_editorInfo->currentState == EEditorState::CommandMode)
	{
		this->m_editorInfo->currentState = EEditorState::None;
	}
}

void Hush::CommandPanel::SubmitCommand(uint32_t command, const std::string_view &textCmd)
{
	switch (static_cast<EBuiltinCommands>(command))
	{
	case EBuiltinCommands::AddEntity: {
		if (textCmd.empty())
		{
			// Open the entity search panel or create a new one
			constexpr const char *addEntityHelp =
				"The add-entity command should be followed by the name of the entity you want to add!";
			constexpr float notificationTime = 3.f;
			constexpr ToastNotification::EToastType type = ToastNotification::EToastType::Info;
			Entity entity = this->m_activeScene->CreateEntity();
			entity.EmplaceComponent<ToastNotification>(addEntityHelp, notificationTime, type);
			break;
		}
		// Interpret the rest of the text command as the name of the entity to add
		Entity createdEntity = this->m_activeScene->CreateEntityWithName(textCmd);
		createdEntity.AddComponent<WorldTransform>();
		createdEntity.AddComponent<LocalTransform>();
		UI::Get().GetPanel<InspectorPanel>().SetInspectTarget(createdEntity.GetId());
		break;
	}
	case EBuiltinCommands::FindEntity:
		this->m_currState = EState::SearchMode;
		this->m_keyboardFocusSet = true;
		return;
	case EBuiltinCommands::AddComponent:
		this->m_currState = EState::AddComponentMode;
		this->m_keyboardFocusSet = true;
		return;
	case EBuiltinCommands::AddSystem:
		this->m_currState = EState::AddSystemMode;
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

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
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
		this->m_selectedCommandIdx =
			MathUtils::CircleBack(nextIdx, 0, static_cast<uint32_t>(BUILT_IN_COMMANDS.size() - 1));
	}
	constexpr float panelHeightOffset = 2.0f;
	constexpr float backgroundAlpha = 0.5f;
	ImGui::SetNextWindowSize({this->m_commandPanelWidth, this->m_commandPanelHeight * panelHeightOffset});
	ImGui::SetNextWindowPos(
		{this->m_commandPanelPos.x, this->m_commandPanelPos.y - (this->m_commandPanelHeight * panelHeightOffset)});
	ImGui::SetNextWindowBgAlpha(backgroundAlpha);
	ImGui::Begin("Available commands", nullptr, ImGuiWindowFlags_NoCollapse);

	// Calculate table dimensions
	const size_t totalCommands = this->m_currentlyAvailableCommands.size();
	if (totalCommands == 0)
	{
		ImGui::End();
		return;
	}

	// Determine number of columns based on window width
	constexpr float itemWidth = 150.0f;
	constexpr float spacing = 10.0f;
	const float availableWidth = ImGui::GetContentRegionAvail().x;
	const int32_t numColumns = std::max(1, static_cast<int32_t>(availableWidth / (itemWidth + spacing)));
	const auto numRows = static_cast<int32_t>((totalCommands + numColumns - 1) / numColumns);

	ImDrawList *drawList = ImGui::GetWindowDrawList();

	// Create table
	if (ImGui::BeginTable("CommandTable", numColumns, ImGuiTableFlags_None | ImGuiTableFlags_SizingStretchSame))
	{
		// Setup columns
		for (int col = 0; col < numColumns; col++)
		{
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch);
		}

		// Populate table rows
		size_t commandIdx = 0;
		for (int32_t row = 0; row < numRows; row++)
		{
			ImGui::TableNextRow();

			for (int32_t col = 0; col < numColumns && commandIdx < totalCommands; col++)
			{
				ImGui::TableSetColumnIndex(col);

				const std::string_view &command = this->m_currentlyAvailableCommands.at(commandIdx);
				bool hovered = false;
				bool forceHover = this->m_selectedCommandIdx == static_cast<int>(commandIdx);
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
					uint32_t commandHash =
						Hashing::Fnv1a(pureCommand.data(), static_cast<uint32_t>(pureCommand.size()));
					this->SubmitCommand(commandHash, cmdText);
				}

				commandIdx++;
			}
		}

		ImGui::EndTable();
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
	PopupListState popupState{};
	popupState.label = "Add Component";
	popupState.leftInstruction = "Select a component to add";
	popupState.selected = this->m_selectedItem;
	popupState.filter = &(this->m_searchInputText[0]);
	popupState.inputId = "##Search";
	popupState.hint = "i.e Rigidbody";

	// Find all components
	const std::vector<Entity::EntityId> &registeredComps = this->m_activeScene->GetAllRegisteredComponents();
	std::vector<Entity::EntityId> allComponentIds{registeredComps.begin(), registeredComps.end()};
	popupState.options.reserve(allComponentIds.size());
	for (Entity::EntityId id : allComponentIds)
	{
		Entity ent = this->m_activeScene->EntityFromIdUnchecked(id);
		popupState.options.emplace_back(ent.GetKey());
	}

	// BACKLOG: We look scripting components up by name here, but we should probably look them up by id or add
	// the Inspectable component to them on registration instead. Component memory lives in the flecs storage,
	// not the scripting's heap or stack.
	if (this->m_scriptingHost != nullptr)
	{
		std::vector<ScriptingRegisteredTypeInfo> &scriptedComponents = this->m_scriptingHost->GetAvailableComponents();
		allComponentIds.reserve(allComponentIds.size() + scriptedComponents.size());
		for (const ScriptingRegisteredTypeInfo &typeInfo : scriptedComponents)
		{
			auto nameView = std::string_view(static_cast<const char *>(typeInfo.name));
			Entity::EntityId compId = this->m_activeScene->Lookup(nameView);
			if (compId == Entity::INVALID_ENTITY_ID)
			{
				// Warn
				continue;
			}
			allComponentIds.emplace_back(compId);
			popupState.options.emplace_back(typeInfo.name);
		}
	}

	Entity &inspectedEntity = inspectTarget.value();

	struct PopupCtx
	{
		Entity *entityRef;
		const std::vector<Entity::EntityId> *compsArr;
	};
	PopupCtx popupCtx{.entityRef = &inspectedEntity, .compsArr = &allComponentIds};

	popupState.ctx = &popupCtx;

	popupState.onElementClicked = [](size_t idx, PopupListState *state) {
		auto *localCtx = reinterpret_cast<PopupCtx *>(state->ctx);
		Entity::EntityId compId = localCtx->compsArr->at(idx);
		localCtx->entityRef->AddComponentRaw(compId);
	};

	DrawSearchPopup(&popupState);

	this->m_selectedItem = popupState.selected;
}

void Hush::CommandPanel::DrawSearchPopup(PopupListState *state)
{
	auto &scenePanel = UI::Get().GetPanel<ScenePanel>();
	glm::u32vec2 scenePanelSize = scenePanel.GetPanelSize();

	// At least 1/3 of the scene width
	ImVec2 minPopupSize = {(float)scenePanelSize.x / 3.f, (float)scenePanelSize.y / 2.f};
	ImGui::SetNextWindowSize(minPopupSize);

	UI::BeginCenterPopup(state->label, true);
	ImGui::Text("%s", state->leftInstruction);
	if (this->m_keyboardFocusSet)
	{
		ImGui::SetKeyboardFocusHere();
		memset(this->m_searchInputText, 0, MAX_TEXT_INPUT_STR_SIZE);
	}
	bool receivedInput = false;
	UI::InputTextWithHint(state->inputId, state->hint, static_cast<char *>(this->m_searchInputText),
						  MAX_TEXT_INPUT_STR_SIZE, true, &receivedInput);

	if (receivedInput)
	{
		state->selected = -1;
	}

	std::string filterStr = state->filter;

	ImDrawList *drawList = ImGui::GetWindowDrawList();
	bool hovered = false;
	std::vector<size_t> indices;
	size_t maxOptionsSize = state->options.size();
	if (filterStr.empty())
	{
		for (size_t i = 0; i < state->options.size(); i++)
		{
			if (UI::CustomSelectable(state->options[i].data(), &hovered, drawList, i == state->selected))
			{
				state->onElementClicked(i, state);
				state->selected = -1;
				this->CloseCommandMode();
			}
		}
	}
	else
	{
		indices =
			ArrayUtils::FuzzyFindIndices<std::vector<std::string_view>, std::string_view>(state->options, filterStr);
		maxOptionsSize = indices.size();
		for (size_t i = 0; i < indices.size(); i++)
		{
			size_t indexOnOptionsArr = indices[i];
			if (UI::CustomSelectable(state->options[indexOnOptionsArr].data(), &hovered, drawList,
									 i == state->selected))
			{
				state->onElementClicked(indexOnOptionsArr, state);
				state->selected = -1;
				this->CloseCommandMode();
			}
		}
	}

	ImGui::End();
	if (maxOptionsSize <= 0)
	{
		return;
	}

	if (ImGui::IsKeyPressed(ImGuiKey_DownArrow, true) || ImGui::IsKeyPressed(ImGuiKey_Tab, true))
	{
		state->selected++;
		state->selected = state->selected % static_cast<int32_t>(maxOptionsSize);
	}
	else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow, true) ||
			 (ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_Tab, true)))
	{
		state->selected--;
		if (state->selected < 0)
		{
			state->selected = static_cast<int32_t>(maxOptionsSize - 1);
		}
	}
}

void Hush::CommandPanel::FindEntityPopup(const char *overrideLabel)
{
	PopupListState state{};
	state.label = overrideLabel != nullptr ? overrideLabel : "Entity search";
	state.filter = &(this->m_searchInputText[0]);
	state.selected = this->m_selectedItem;
	state.inputId = "##SearchEntity";
	state.hint = "i.e Player";
	state.leftInstruction = "Find any entity";

	// TODO: Allocate this on a small arena
	std::vector<Entity::EntityId> idsByIdx;

	// Then find all entities in the scene here
	Query<Entity::Name, WorldTransform> query = this->m_activeScene->CreateQuery<Entity::Name, WorldTransform>();
	query.Each([&state, &idsByIdx](Entity &entity, Entity::Name &name,
								   [[maybe_unused]]
								   WorldTransform &transform) {
		(void)(entity);
		std::string_view currEntityName = name.GetName();
		state.options.emplace_back(currEntityName);
		idsByIdx.emplace_back(entity.GetId());
	});

	state.ctx = &idsByIdx;
	state.onElementClicked = [](size_t idx, PopupListState *state) {
		auto *localIdsByIdx = reinterpret_cast<std::vector<Entity::EntityId> *>(state->ctx);
		Entity::EntityId targetId = localIdsByIdx->at(idx);
		// TODO: Turn this into an event the inspector panel listens for
		UI::Get().GetPanel<InspectorPanel>().SetInspectTarget(targetId);
	};

	DrawSearchPopup(&state);

	// Update the selected item on the global state
	this->m_selectedItem = state.selected;
}

void Hush::CommandPanel::RenderEntitySelectable(const std::string_view &entityName, Entity::EntityId entityId)
{
	auto narrowedEntity = static_cast<int32_t>(entityId);
	ImGui::PushID(narrowedEntity);
	if (ImGui::Selectable(entityName.data()))
	{
		UI::Get().GetPanel<InspectorPanel>().SetInspectTarget(entityId);
		this->CloseCommandMode();
	}
	ImGui::PopID();
}
