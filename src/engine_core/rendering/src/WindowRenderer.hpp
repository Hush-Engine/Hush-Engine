/*! \file WindowRenderer.hpp
	\author Kyn21kx
	\date 2024-02-26
	\brief Instances of this class are a wrapper around an SDL renderer and event handler, includes disposing behaviours
*/

#pragma once

// Let's tell SDL we got main covered
#include <SDL2/SDL_video.h>
#include <cstdint>
#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include <InputManager.hpp>
#include <memory>

#include "RHI/IGraphicsDevice.hpp"
#include "RHI/GraphicsResources.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "Renderer.hpp"
#include "RenderGraph/RenderGraph.hpp"

constexpr int DEFAULT_WINDOW_HEIGHT = 720;
constexpr int DEFAULT_WINDOW_WIDTH = 1280;

namespace Hush
{
	class WindowRenderer
	{
	public:
		WindowRenderer(const char *windowName, Scene *activeScene) noexcept;

		WindowRenderer(WindowRenderer &&other) = delete;

		WindowRenderer(const WindowRenderer &other) = delete;

		WindowRenderer &operator=(const WindowRenderer &) = delete;

		WindowRenderer &operator=(WindowRenderer &&) = delete;

		void HandleEvents(bool *applicationRunning);

		~WindowRenderer();

		IRenderer *GetInternalRenderer() noexcept;

		[[nodiscard]]
		bool IsActive() const noexcept;

		void GetWindowSize(int32_t *width, int32_t *height);

		[[nodiscard]]
		Hush::RenderGraph::RenderGraph &GetRenderGraph()
		{
			return *this->m_renderGraph;
		}

		[[nodiscard]]
		Hush::Graphics::IGraphicsDevice *GetGraphicsDevice() noexcept
		{
			return this->m_windowRenderer.get();
		}

	private:
		/// @brief Pointer that represents the unique instance of an SDL window associated with this context
		/// (This is declared as a raw pointer for compatibility with C)
		SDL_Window *m_windowPtr = nullptr;

		SDL_Renderer *m_rendererPtr = nullptr;

		std::unique_ptr<Hush::Graphics::IGraphicsDevice> m_windowRenderer;

		std::unique_ptr<Hush::RenderGraph::RenderGraph> m_renderGraph;

		bool m_isActive = false;

		bool InitSDLIfNotStarted() noexcept;

		void CheckWindowState(SDL_WindowEvent windowEvent, bool *isActive) noexcept;

		constexpr uint32_t GetInitialRendererFlags()
		{
			return SDL_WINDOW_MOUSE_GRABBED | SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_RESIZABLE;
		}
	};

} // namespace Hush
