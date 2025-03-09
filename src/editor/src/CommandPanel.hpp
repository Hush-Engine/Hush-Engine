#pragma once

#include "IEditorPanel.hpp"
#include "time/Timer.hpp"
#include <string>
#include <string_view>

namespace Hush {
	class CommandPanel : public IEditorPanel {
	public:
		void OnRender() override;
		
	private:
		static inline constexpr std::string_view DEFAULT_CMD_PANEL_TEXT = "Type \":\" to enter command mode";

		void HandleInput();
		
		void TypeCommand();
		
		std::string m_panelText = DEFAULT_CMD_PANEL_TEXT.data();

		bool m_acceptText = false;
		
		Timer m_inputTimer;
	};
}

