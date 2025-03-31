#pragma once

#include "IEditorPanel.hpp"
#include "Scene.hpp"
#include "imgui/imgui.h"
#include <cstdint>
#include <string>
#include <string_view>

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

			IsPopupMode = SearchMode | AddComponentMode
		};

		void Init(Scene *activeScene) noexcept override;

		void OnRender() override;

	private:
		static inline constexpr std::string_view DEFAULT_CMD_PANEL_TEXT = "Type \":\" to enter command mode";

		void HandleInput();

		void TypeCommand();

		void UpdateCommandList();

		void CloseCommandMode();

		void AddComponentPopup();
		
		void FindEntityPopup(const char* overrideLabel = nullptr);

		void RenderEntitySelectable(const std::string_view &entityName, Entity::EntityId entityId);

		void SubmitCommand(uint32_t command, const std::string_view& textCmd);

		void RebuildAvailableCommands();
		
		std::string m_panelText = DEFAULT_CMD_PANEL_TEXT.data();

		EState m_currState = EState::None;

		int32_t m_selectedCommandIdx = -1;

		float m_commandPanelWidth = 0.0F;

		float m_commandPanelHeight = 0.0F;

		ImVec2 m_commandPanelPos;

		Scene *m_activeScene;

		bool m_keyboardFocusSet = false;

		std::vector<std::string_view> m_currentlyAvailableCommands;

		static constexpr size_t MAX_ALLOWED_ENTITY_NAME = 30;
		char m_searchInputText[MAX_ALLOWED_ENTITY_NAME] = {0};
	};
} // namespace Hush
