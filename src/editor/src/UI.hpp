/*! \file UI.hpp
	\author Kyn21kx
	\date 2024-05-25
	\brief UI methods and common components (to be moved to its own project, please)
*/

#pragma once

#include "IEditorPanel.hpp"
#include "Scene.hpp"
#include "imgui/imgui.h"
#include <memory>
#include <unordered_map>
#include <typeindex>

namespace Hush
{
	class Transform;
	class UI
	{
	public:
		enum class ESerializableComponentType
		{
			Unkwon = 0,
			Transform
		};

		UI();

		void Init(Scene *parentScene);

		void DrawPanels(float deltaTime);

		template <class T>
		[[nodiscard]]
		T &GetPanel() const noexcept
		{
			return *static_cast<T *>(this->m_activePanels.at(typeid(T)).get());
		}

		static bool Spinner(const char *label, float radius, int thickness,
							const uint32_t &color = 3435973836u /*Default button color*/);

		static bool InputTextWithHint(const char *label, const char *hint, char *buffer, size_t size,
									  bool focusOnInput);

		static bool BeginCenterPopup(const char *label, bool transparent = false);

		static bool CustomSelectable(const char *label, bool *isHovered, ImDrawList *drawList, bool forceHover = false);

		static bool BeginToolBar();

		static ImGuiID DockSpace(const char *dockspaceId, const char *name, ImGuiDockNodeFlags additionalFlags = 0);

		static UI &Get();

		static inline bool S_INITIALIZED = false;
	private:
		static void DrawPlayButton();

		void SetupImGuiStyle();

		// NOLINTNEXTLINE
		static inline UI *s_instance;

		template <class T>
		static std::unique_ptr<T> CreatePanel(Scene *activeScene)
		{
			auto result = std::make_unique<T>();
			static_cast<IEditorPanel *>(result.get())->Init(activeScene);
			return result;
		}
		std::unordered_map<std::type_index, std::unique_ptr<IEditorPanel>> m_activePanels;
	};

} // namespace Hush
