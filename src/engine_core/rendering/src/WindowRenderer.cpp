#include "WindowRenderer.hpp"
#include "WindowManager.hpp"
#include "Logger.hpp"
#include "Vulkan/VulkanRenderer.hpp"
#include "Assertions.hpp"

Hush::WindowRenderer::WindowRenderer(const char *windowName) noexcept
{
    if (!InitSDLIfNotStarted())
    {
        Hush::LogFormat(Hush::ELogLevel::Critical, "SDL initialization failed with error {}!", SDL_GetError());
        return;
    }

    if (WindowManager::GetMainWindow() == nullptr)
    {
        // Set this window as the main one
        WindowManager::SetMainWindow(this);
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

    if (this->m_rendererPtr == nullptr)
    {
        Hush::LogError(std::string("SDL renderer creation failed! ") + SDL_GetError());
    }

    this->m_windowRenderer = std::make_unique<Hush::VulkanRenderer>(this->m_windowPtr);
    this->m_windowRenderer->CreateSwapChain(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);
    this->m_windowRenderer->InitImGui();
    this->m_windowRenderer->InitRendering();
    this->m_isActive = true;
}

void Hush::WindowRenderer::HandleEvents(bool *applicationRunning)
{
    SDL_Event event;
    KeyCode code = 0;
    InputManager::ResetMouseAcceleration();
    SDL_PollEvent(&event);
    // Forward event to the renderer
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
    case SDL_MOUSEMOTION:
        InputManager::SendMouseMovementEvent(event.motion.x, event.motion.y, event.motion.xrel, event.motion.yrel);
        break;
    case SDL_WINDOWEVENT:
        CheckWindowState(event.window, &this->m_isActive);
        break;
    default:
        break;
    }
    this->m_windowRenderer->HandleEvent(&event);
}

Hush::WindowRenderer::~WindowRenderer()
{
    SDL_DestroyWindow(this->m_windowPtr);
    SDL_DestroyRenderer(this->m_rendererPtr);
    SDL_Quit();
}

Hush::IRenderer *Hush::WindowRenderer::GetInternalRenderer() noexcept
{
    return this->m_windowRenderer.get();
}

bool Hush::WindowRenderer::IsActive() const noexcept
{
    return this->m_isActive;
}

bool Hush::WindowRenderer::InitSDLIfNotStarted() noexcept
{
    if (SDL_WasInit(SDL_INIT_EVERYTHING) != 0)
    {
        return true;
    }
    int rc = SDL_Init(SDL_INIT_EVERYTHING);
    SDL_SetMainReady();
    return rc == 0;
}

void Hush::WindowRenderer::CheckWindowState(const SDL_WindowEvent windowEvent, bool *isActive) noexcept
{
    switch (windowEvent.event)
    {
    case SDL_WINDOWEVENT_MINIMIZED:
        *isActive = false;
        break;
    case SDL_WINDOWEVENT_RESTORED:
        *isActive = true;
        break;
    }
}