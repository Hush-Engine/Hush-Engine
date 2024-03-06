#include "WindowRenderer.hpp"

#include "log/Logger.hpp"
#include "rendering/VulkanRenderer.hpp"

WindowRenderer::WindowRenderer(const char *windowName) noexcept
{
    if (!InitSDLIfNotStarted())
    {
        Hush::LogFormat(Hush::ELogLevel::Critical, "SDL initialization failed with error {}!", SDL_GetError());
        return;
    }
    // Now create the window
    uint32_t defaultFlag = SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE;
    const int defaultWindowIndex = -1;

    this->m_windowPtr = SDL_CreateWindow(windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                         DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, defaultFlag);
    if (this->m_windowPtr == nullptr)
    {
        Hush::LogError("SDL window creation failed!");
        return;
    }
    this->m_rendererPtr = SDL_CreateRenderer(this->m_windowPtr, defaultWindowIndex, GetInitialRendererFlags());

    this->m_windowRenderer = std::make_unique<Hush::VulkanRenderer>(this->m_windowPtr);
}

void WindowRenderer::HandleEvents(bool *applicationRunning)
{
    SDL_Event event;
    KeyCode code = 0;
    SDL_PollEvent(&event);
    switch (event.type)
    {
    case SDL_QUIT:
        *applicationRunning = false;
        break;
    case SDL_KEYDOWN:
        code = event.key.keysym.scancode;
        InputManager::SendKeyEvent(code, EKeyState::Pressed);
        break;
    case SDL_KEYUP:
        code = event.key.keysym.scancode;
        InputManager::SendKeyEvent(code, EKeyState::Released);
        break;
    case SDL_MOUSEBUTTONDOWN:
        InputManager::SendMouseButtonEvent(event.button.button, EKeyState::Pressed);
        break;
    case SDL_MOUSEBUTTONUP:
        InputManager::SendMouseButtonEvent(event.button.button, EKeyState::Released);
        break;
    default:
        break;
    }
}

WindowRenderer::~WindowRenderer()
{
    SDL_DestroyWindow(this->m_windowPtr);
    SDL_DestroyRenderer(this->m_rendererPtr);
    SDL_Quit();
}

bool WindowRenderer::InitSDLIfNotStarted() noexcept
{
    if (SDL_WasInit(SDL_INIT_EVERYTHING) != 0)
    {
        return true;
    }
    int rc = SDL_Init(SDL_INIT_EVERYTHING);
    SDL_SetMainReady();
    return rc == 0;
}
