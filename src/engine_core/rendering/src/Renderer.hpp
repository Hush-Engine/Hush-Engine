/*! \file Renderer.hpp
	\author Alan Ramirez Herrera
	\date 2024-03-03
	\brief Abstract renderer
*/

#pragma once

#include <SDL2/SDL.h>
#include <cstdint>

namespace Hush
{
	/// @brief A common interface for renderers, Hush supports many graphics APIs, and this is the interface
	/// that allows us to standardize all of them...
	/// All renderers MUST bind to SDL and ImGUI, the latter can be done through the IImGuiForwarder interface
	class IRenderer
	{
	public:
		IRenderer(void *windowContext)
		{
			(void)windowContext;
		}

		IRenderer(const IRenderer &) = delete;
		IRenderer &operator=(const IRenderer &) = delete;
		IRenderer(IRenderer &&) = delete;
		IRenderer &operator=(IRenderer &&) = delete;

		virtual ~IRenderer() = default;

		virtual void CreateSwapChain(uint32_t width, uint32_t height) = 0;

		virtual void InitImGui() = 0;

		virtual void Draw(float delta) = 0;

		/// @brief Each renderer will have to implement a way of updating all the objects
		/// inside of the scene, these are instances of the IRenderableNode, which is a common interface
		/// for all renderers, but additional render data (i.e drawContext) might be needed by their underlying
		/// implementation (see VulkanMeshNode for an example)
		virtual void UpdateSceneObjects(float delta) = 0;

		/// @brief Initializes all the internal structures needed to begin rendering, call after a swapchain has been
		/// created!
		virtual void InitRendering() = 0;

		virtual void NewUIFrame() const noexcept = 0;

		virtual void EndUIFrame() const noexcept = 0;

		virtual void HandleEvent(const SDL_Event *event) noexcept = 0;

		[[nodiscard]]
		virtual void *GetWindowContext() const noexcept = 0;
	};
} // namespace Hush
