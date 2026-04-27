/*! \file WindowRenderer.hpp
	\author Kyn21kx
	\date 2024-02-26
	\brief Instances of this class are a wrapper around an SDL renderer and event handler, includes disposing behaviours
*/

#pragma once

// Let's tell SDL we got main covered
#include <SDL3/SDL_video.h>
#include <cstdint>
#define SDL_MAIN_HANDLED

#include <SDL3/SDL.h>
#include <InputManager.hpp>
#include <functional>
#include <memory>

#include "RHI/IGraphicsDevice.hpp"
#include "RHI/GraphicsResources.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "Renderer.hpp"
#include "RenderGraph/RenderDevice.hpp"

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

		void HandleEvents(bool *applicationRunning, const SDL_Event &event);

		~WindowRenderer();

		IRenderer *GetInternalRenderer() noexcept;

		[[nodiscard]]
		bool IsActive() const noexcept;

		[[nodiscard]]
		glm::u32vec2 GetWindowSize() noexcept;

		[[nodiscard]]
		Hush::RenderGraph::RenderGraph &GetRenderGraph()
		{
			return this->m_renderDevice->GetRenderGraph();
		}

		[[nodiscard]]
		Hush::RenderGraph::RenderDevice &GetRenderDevice()
		{
			return *this->m_renderDevice;
		}

		[[nodiscard]]
		Hush::Graphics::IGraphicsDevice *GetGraphicsDevice() noexcept
		{
			return this->m_windowRenderer.get();
		}

		/// @brief Returns the raw SDL window pointer for use by subsystems (e.g. ImGui).
		[[nodiscard]]
		SDL_Window *GetSDLWindow() noexcept
		{
			return this->m_windowPtr;
		}

	private:
		/// @brief Pointer that represents the unique instance of an SDL window associated with this context
		/// (This is declared as a raw pointer for compatibility with C)
		SDL_Window *m_windowPtr = nullptr;

		SDL_Renderer *m_rendererPtr = nullptr;

		std::unique_ptr<Hush::Graphics::IGraphicsDevice> m_windowRenderer;

		std::unique_ptr<Hush::RenderGraph::RenderDevice> m_renderDevice;

		bool m_isActive = false;

		bool InitSDLIfNotStarted() noexcept;

		void CheckWindowState(SDL_WindowEvent windowEvent, bool *isActive) noexcept;

		constexpr uint32_t GetInitialRendererFlags()
		{
			return SDL_WINDOW_MOUSE_GRABBED | SDL_WINDOW_MOUSE_CAPTURE | SDL_WINDOW_RESIZABLE;
		}
	};

} // namespace Hush
