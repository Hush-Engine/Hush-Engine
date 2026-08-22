/*! \file GamePanel.hpp
	\author Kyn21kx
	\date 2026-08-21
	\brief ImGui Panel where the game view is rendered, projected by an entity's Camera component
*/

#pragma once

#include "Entity.hpp"
#include "IEditorPanel.hpp"
#include <glm/glm.hpp>
#include <cstdint>

namespace Hush
{
	class GamePanel final : public IEditorPanel
	{
	public:
		void Init(Scene *activeScene) noexcept override;
		void OnRender(float deltaTime) noexcept override;

		/// @brief Set the native texture view handle to display the rendered game view.
		/// For WebGPU this should be a WGPUTextureView cast to void*.
		void SetGameTextureView(void *nativeTextureView, uint32_t width, uint32_t height) noexcept;

		/// @brief Returns the current content size of the game panel in pixels.
		/// This is the size that the game view render texture should match.
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
		void *m_gameTextureView = nullptr;
		uint32_t m_textureWidth = 0;
		uint32_t m_textureHeight = 0;

		bool m_resized = false;
		Entity m_bridgeEntity;
		ComponentRef m_panelSizeRef;

		Scene *m_activeScene = nullptr;
	};
} // namespace Hush
