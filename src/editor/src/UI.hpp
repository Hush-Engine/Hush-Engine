/*! \file UI.hpp
	\author Kyn21kx
	\date 2024-05-25
	\brief UI methods and common components (to be moved to its own project, please)
*/

#pragma once

#include "IEditorPanel.hpp"
#include "ScenePanel.hpp"
#include "Scene.hpp"
#include "RHI/ShaderCompiler.hpp"
#include "imgui/imgui.h"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <typeindex>

namespace Hush
{
	struct Transform;
	class ScriptingHost;

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

		/// @brief Forward the native scene texture view to the ScenePanel for display.
		/// @param nativeTextureView Native texture view handle (e.g. WGPUTextureView cast to void*).
		/// @param width Texture width in pixels.
		/// @param height Texture height in pixels.
		void SetSceneTextureView(void *nativeTextureView, uint32_t width, uint32_t height) const noexcept
		{
			GetPanel<ScenePanel>().SetSceneTextureView(nativeTextureView, width, height);
		}

		template <class T>
		[[nodiscard]]
		T &GetPanel() const noexcept
		{
			return *static_cast<T *>(this->m_activePanels.at(typeid(T)).get());
		}

		static bool Spinner(const char *label, float radius, int thickness,
							const uint32_t &color = 3435973836u /*Default button color*/);

		static bool InputTextWithHint(const char *label, const char *hint, char *buffer, size_t size, bool focusOnInput,
									  bool *receivedInput);

		static bool BeginCenterPopup(const char *label, bool transparent = false);

		static bool CustomSelectable(const char *label, bool *isHovered, ImDrawList *drawList, bool forceHover = false);

		static bool Vec3Edit(const char *label, float v[3], float step = 0.01f, float stepFast = 0.1f,
							 const char *format = "%.3f");

		static bool Vec4Edit(const char *label, float v[4], float step = 0.01f, float stepFast = 0.1f,
							 const char *format = "%.3f");

		static bool BeginToolBar();

		/// @brief Begin a popup attached to a button showing @p label. Call @ref FlagItem inside.
		/// @return true if the popup is open and FlagItems should be rendered.
		static bool FlagsBegin(const char *label);

		/// @brief End the popup opened by @ref FlagsBegin.
		static void FlagsEnd();

		/// @brief Render a checkbox to toggle a single flag inside a FlagsBegin/FlagsEnd block.
		/// @return true if the flag was toggled this frame.
		template <class T>
		static bool FlagItem(const char *label, T flagValue, T *flags)
		{
			bool active = Bitwise::HasCompositeFlag(*flags, flagValue);
			ImGui::PushID(label);
			if (ImGui::Checkbox(label, &active))
			{
				if (active)
				{
					*flags |= flagValue;
				}
				else
				{
					*flags = static_cast<T>(static_cast<uint64_t>(*flags) & ~static_cast<uint64_t>(flagValue));
				}
				ImGui::PopID();
				return true;
			}
			ImGui::PopID();
			return false;
		}

		static ImGuiID DockSpace(const char *dockspaceId, const char *name, ImGuiDockNodeFlags additionalFlags = 0);

		static UI &Get();

		static inline bool S_INITIALIZED = false;

	private:
		static void DrawPlayButton();

		static bool DrawVecComponent(const char *id, float *value, const ImVec4 &color, float step, const char *format,
									 float fieldWidth, float buttonWidth);

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
