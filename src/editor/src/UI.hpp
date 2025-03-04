/*! \file UI.hpp
	\author Kyn21kx
	\date 2024-05-25
	\brief UI methods and common components (to be moved to its own project, please)
*/

#pragma once

#include "IEditorPanel.hpp"
#include <memory>
#include <unordered_map>
#include <typeindex>

namespace Hush
{
	class UI
	{
	public:
		UI();

		void DrawPanels();

		template <class T>
		[[nodiscard]]
		T &GetPanel() const noexcept
		{
			return *static_cast<T *>(this->m_activePanels.at(typeid(T)).get());
		}

		static bool Spinner(const char *label, float radius, int thickness,
							const uint32_t &color = 3435973836u /*Default button color*/);

		static bool BeginToolBar();

		static void DockSpace();

		static UI &Get();

	private:
		static void DrawPlayButton();

		static inline UI *s_instance;

		template <class T>
		static std::unique_ptr<T> CreatePanel()
		{
			return std::make_unique<T>();
		}
		std::unordered_map<std::type_index, std::unique_ptr<IEditorPanel>> m_activePanels;
	};

} // namespace Hush