/*! \file ScenePanel.hpp
	\author Leonidas Gonzalez
	\date 2024-05-27
	\brief ImGui Panel where the Scene is rendered
*/

#pragma once

#include "Entity.hpp"
#include "IEditorPanel.hpp"
#include <glm/glm.hpp>

namespace Hush
{
	class ScenePanel final : public IEditorPanel
	{
	public:
		void Init(Scene *activeScene) noexcept override;
		void OnRender(float deltaTime) noexcept override;

		/// @brief Set the native texture view handle to display the rendered scene.
		/// For WebGPU this should be a WGPUTextureView cast to void*.
		void SetSceneTextureView(void *nativeTextureView, uint32_t width, uint32_t height) noexcept;

		/// @brief Returns the current content size of the scene panel in pixels.
		/// This is the size that the scene render texture should match.
		/// Returns (0, 0) before the first frame.
		[[nodiscard]]
		glm::u32vec2 GetPanelSize() const noexcept;

		/// @brief Returns true if the panel content region changed size since
		/// the last call to ConsumeResized().  After calling ConsumeResized()
		/// the flag is cleared until the next size change.
		[[nodiscard]]
		bool WasResized() const noexcept
		{
			return m_resized;
		}

		/// @brief Returns and clears the resized flag.
		bool ConsumeResized() noexcept
		{
			bool r = m_resized;
			m_resized = false;
			return r;
		}

	private:
		void *m_sceneTextureView = nullptr;
		uint32_t m_textureWidth = 0;
		uint32_t m_textureHeight = 0;

		/// Set to true whenever m_panelSize changes.
		bool m_resized = false;
		// Entity that represents the scene panel in the ECS, it's used to communicate with other systems
		Entity m_bridgeEntity;
		ComponentRef m_panelSizeRef;
	};
} // namespace Hush
