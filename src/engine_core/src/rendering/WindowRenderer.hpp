/*! \file WindowRenderer.hpp
	\author Kyn21kx
	\date 2024-02-26
	\brief Instances of this class are a wrapper around an SDL renderer and event handler, includes disposing behaviours
*/

#pragma once

//Let's tell SDL we got main covered
#define SDL_MAIN_HANDLED

#include <SDL2/SDL.h>
#include "Logger.hpp"

constexpr int DEFAULT_WINDOW_HEIGHT = 1080;
constexpr int DEFAULT_WINDOW_WIDTH = 1920;

class WindowRenderer
{
public:
	WindowRenderer(const char* windowName) noexcept;

	WindowRenderer(WindowRenderer &&other) = default;

	WindowRenderer(const WindowRenderer &other) = default;

	WindowRenderer &operator=(const WindowRenderer &) = default;

    WindowRenderer &operator=(WindowRenderer &&) = default;

	~WindowRenderer();

private:
	/// <summary>
	/// Pointer that represents the unique instance of an SDL window associated with this context
	/// (This is declared as a raw pointer for compatibility with C)
	/// </summary>
	SDL_Window *m_windowPtr = nullptr;

	SDL_Renderer *m_rendererPtr = nullptr;
	
	bool InitSDLIfNotStarted() noexcept;

	constexpr uint32_t GetInitialRendererFlags()
    {
        return SDL_WindowFlags::SDL_WINDOW_VULKAN | SDL_WindowFlags::SDL_WINDOW_SHOWN;
	}

};