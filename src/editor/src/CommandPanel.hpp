#pragma once

#include "IEditorPanel.hpp"
#include "Scene.hpp"
#include "systems/CommandSystem.hpp"
#include "imgui/imgui.h"
#include <string>
#include <string_view>

namespace Hush {
	class CommandPanel : public IEditorPanel {
	public:
		enum class EState 
		{
			None = 0,
			Editing,
			ForceFocus	
		};

		
		enum class EBuiltinCommands : int32_t {
			AddEntity,
			FindEntity,
			AddComponent,
			Help
		};

		void Init(Scene* activeScene) noexcept override;
		
		void OnRender() override;
		
	private:
		static inline constexpr std::string_view DEFAULT_CMD_PANEL_TEXT = "Type \":\" to enter command mode";

		void HandleInput();
		
		void TypeCommand();

		void UpdateCommandList();
		
		void CloseCommandMode();
		
		void SubmitCommand(EBuiltinCommands command, std::string_view textCmd);
		
		std::string m_panelText = DEFAULT_CMD_PANEL_TEXT.data();

		EState m_currState = EState::None;

		int32_t m_selectedCommandIdx = -1;

		float m_commandPanelWidth = 0.0F;
		
		float m_commandPanelHeight = 0.0F;

		ImVec2 m_commandPanelPos;

		Scene* m_activeScene;
		
	};
}

