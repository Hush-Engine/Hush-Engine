#pragma once

#include "IEditorPanel.hpp"
#include "Scene.hpp"
#include "ScriptingHost.hpp"
#include "components/EditorInfo.hpp"
#include "imgui/imgui.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Hush
{

	class CommandPanel : public IEditorPanel
	{
	public:
		enum class EState
		{
			None = 0,
			Editing = 0b00000001,
			ForceFocus = 0b00000010,
			SearchMode = 0b00000100,
			AddComponentMode = 0b00001000,
			AddSystemMode = 0b00010000,

			IsPopupMode = SearchMode | AddComponentMode | AddSystemMode
		};

		void Init(Scene *activeScene) noexcept override;

		void OnRender(float deltaTime) override;

	private:
		// For any pop up list, including (later on) the commands themselves
		struct PopupListState;
		using OnElementClickedFn_t = void (*)(size_t, PopupListState *);
		struct PopupListState
		{
			std::vector<std::string_view> options;
			const char *filter;
			int32_t selected;
			const char *label = nullptr;
			const char *leftInstruction = nullptr;
			const char *inputId = nullptr;
			const char *hint = nullptr;
			OnElementClickedFn_t onElementClicked = nullptr;
			void *ctx = nullptr;
		};

		static inline constexpr std::string_view DEFAULT_CMD_PANEL_TEXT = "Type \":\" to enter command mode";

		void HandleInput();

		void TypeCommand();

		void UpdateCommandList();

		void CloseCommandMode();

		void AddComponentPopup();

		void FindEntityPopup(const char *overrideLabel = nullptr);

		void DrawSearchPopup(PopupListState *state);

		void RenderEntitySelectable(const std::string_view &entityName, Entity::EntityId entityId);

		void SubmitCommand(uint32_t command, const std::string_view &textCmd);

		void RebuildAvailableCommands();

		std::string m_panelText = DEFAULT_CMD_PANEL_TEXT.data();

		EState m_currState = EState::None;

		int32_t m_selectedCommandIdx = -1;

		float m_commandPanelWidth = 0.0F;

		float m_commandPanelHeight = 0.0F;

		ImVec2 m_commandPanelPos;

		Scene *m_activeScene;

		EditorInfo *m_editorInfo;

		ScriptingHost *m_scriptingHost;

		// For the selection state of any pop up
		int32_t m_selectedItem = -1;

		bool m_keyboardFocusSet = false;

		std::vector<std::string_view> m_currentlyAvailableCommands;

		static constexpr size_t MAX_TEXT_INPUT_STR_SIZE = 64;
		char m_searchInputText[MAX_TEXT_INPUT_STR_SIZE] = {0};
	};
} // namespace Hush
