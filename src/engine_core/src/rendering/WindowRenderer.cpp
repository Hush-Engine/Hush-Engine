#include "WindowRenderer.hpp"

WindowRenderer::WindowRenderer(const char *windowName) noexcept
{
	if (!InitSDLIfNotStarted())
	{
		LOG_ERROR_LN("SDL initialization failed with error %s!", SDL_GetError());
		return;
	}
	//Now create the window
	uint32_t defaultFlag = 0;
	this->m_windowPtr = SDL_CreateWindow(windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
										 DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, defaultFlag);

    this->m_rendererPtr = SDL_CreateRenderer(this->m_windowPtr, -1, GetInitialRendererFlags());
}

WindowRenderer::~WindowRenderer()
{
    SDL_DestroyWindow(this->m_windowPtr);
	SDL_DestroyRenderer(this->m_rendererPtr);
    SDL_Quit();
}

bool WindowRenderer::InitSDLIfNotStarted() noexcept
{
	if (SDL_WasInit(SDL_INIT_EVERYTHING) == 0)
	{
		return true;
	}
	int rc = SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetMainReady();
	return rc != 0;
}