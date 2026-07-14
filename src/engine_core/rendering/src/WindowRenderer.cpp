#include "WindowRenderer.hpp"
#include "InputManager.hpp"
#include "Platform.hpp"
#include "Renderer.hpp"
#include "WindowManager.hpp"
#include "Logger.hpp"
// #include "Vulkan/VulkanRenderer.hpp"
#include "definitions/KeyCode.hpp"
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_video.h>
#include <imgui/backends/imgui_impl_sdl3.h>

// Graphics backend
#if defined(HUSH_VULKAN_IMPL)
// TODO:
#elif defined(HUSH_WEBGPU_IMPL)
#include "WebGPU/WebGPUGraphicsDevice.hpp"
#endif

static inline Hush::Graphics::EGraphicsAPI GetPreferredGraphicsAPI()
{
	constexpr Hush::EPlatform currentPlatform = Hush::GetCurrentPlatform();

	switch (currentPlatform)
	{
	case Hush::EPlatform::Win64:
		return Hush::Graphics::EGraphicsAPI::D3D12;
	case Hush::EPlatform::Linux:
		return Hush::Graphics::EGraphicsAPI::Vulkan;
	case Hush::EPlatform::OSX:
		return Hush::Graphics::EGraphicsAPI::Metal;
	case Hush::EPlatform::Emscripten:
		return Hush::Graphics::EGraphicsAPI::WebGPU;
	default:
		Hush::LogWarn("Unrecognized platform, defaulting to Vulkan graphics API");
		return Hush::Graphics::EGraphicsAPI::Vulkan;
	}
}

/// @brief Create a graphics device from a given API and window context.
///
static std::unique_ptr<Hush::Graphics::IGraphicsDevice> CreateGraphicsDevice(Hush::Graphics::EGraphicsAPI api,
																			 void *windowHandle)
{
#if defined(HUSH_VULKAN)
	if (api == Hush::Graphics::EGraphicsAPI::Vulkan)
	{
		// return std::make_unique<Hush::Graphics::VulkanRenderer>(windowHandle);
	}
#elif defined(HUSH_WEBGPU_IMPL)
	if (api == Hush::Graphics::EGraphicsAPI::WebGPU)
	{
		return std::make_unique<Hush::Graphics::WebGPUGraphicsDevice>(windowHandle);
	}
#endif // HUSH_VULKAN

#if defined(HUSH_WEBGPU_IMPL)
	Hush::LogWarn("Preferred graphics API is not supported on this platform, falling back to WebGPU");
	return std::make_unique<Hush::Graphics::WebGPUGraphicsDevice>(windowHandle);
#else
	return nullptr;
#endif
}

Hush::WindowRenderer::WindowRenderer(const char *windowName, [[maybe_unused]] Scene *activeScene) noexcept
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
	uint32_t defaultFlag = SDL_WINDOW_RESIZABLE;
	// const int defaultWindowIndex = -1;

	this->m_windowPtr = SDL_CreateWindow(windowName, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, defaultFlag);
	// this->m_windowPtr = SDL_CreateWindow(windowName, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
	// 									 DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, defaultFlag);
	if (this->m_windowPtr == nullptr)
	{
		Hush::LogError("SDL window creation failed!");
		return;
	}
	// this->m_rendererPtr = SDL_CreateRenderer(this->m_windowPtr, defaultWindowIndex, GetInitialRendererFlags());

	// 	if (this->m_rendererPtr == nullptr)
	// 	{
	// 		Hush::ELogLevel severity = ELogLevel::Error;
	// #ifdef HUSH_VULKAN_IMPL
	// 		severity = ELogLevel::Warn;
	// #endif // HUSH_VULKAN_IMPL
	// 		Hush::LogFormat(severity, "SDL renderer creation failed! {}", SDL_GetError());
	// 	}

	this->m_windowRenderer = CreateGraphicsDevice(GetPreferredGraphicsAPI(), this->m_windowPtr);

	this->m_renderDevice = std::make_unique<Hush::RenderGraph::RenderDevice>(this->m_windowRenderer.get());
	this->m_isActive = true;
}

glm::u32vec2 Hush::WindowRenderer::GetWindowSize() noexcept
{
	int32_t width{};
	int32_t height{};

	SDL_GetWindowSize(this->m_windowPtr, &width, &height);

	return {width, height};
}

void Hush::WindowRenderer::HandleEvents(bool *applicationRunning, const SDL_Event &event)
{
	// SDL_Event event;
	KeyCode code = 0;
	// Forward event to ImGui
	// TODO: this shouldn't be here, but this will be done later.
	if (ImGui::GetCurrentContext() != nullptr)
	{
		ImGui_ImplSDL3_ProcessEvent(&event);
	}
	// Forward event to the renderer
	switch (event.type)
	{
	case SDL_EVENT_QUIT:
		*applicationRunning = false;
		break;
	case SDL_EVENT_KEY_DOWN:
		code = SDL_GetScancodeFromKey(event.key.key, nullptr);
		InputManager::SendKeyEvent(code, EKeyState::Pressed);
		break;
	case SDL_EVENT_KEY_UP:
		code = SDL_GetScancodeFromKey(event.key.key, nullptr);
		InputManager::SendKeyEvent(code, EKeyState::Released);
		break;
	case SDL_EVENT_TEXT_INPUT:
		// HACK: directly handle this
		InputManager::SendCharEvent(event.text.text[0]);
		break;
	case SDL_EVENT_MOUSE_BUTTON_DOWN:
		InputManager::SendMouseButtonEvent(event.button.button, EKeyState::Pressed);
		break;
	case SDL_EVENT_MOUSE_BUTTON_UP:
		InputManager::SendMouseButtonEvent(event.button.button, EKeyState::Released);
		break;
	case SDL_EVENT_MOUSE_MOTION:
		InputManager::SendMouseMovementEvent(static_cast<int32_t>(event.motion.x), static_cast<int32_t>(event.motion.y),
											 static_cast<int32_t>(event.motion.xrel),
											 static_cast<int32_t>(event.motion.yrel));
		break;
	case SDL_EVENT_MOUSE_WHEEL:
		// Send 0 as acceleration bc it will be calculated manually
		InputManager::SendWheelEvent(event.wheel.mouse_x, event.wheel.mouse_y);
		break;
	case SDL_EVENT_DROP_FILE:
		if (m_dropCallback && event.drop.data)
		{
			m_dropCallback(std::filesystem::path(event.drop.data));
		}
		break;
	default:
		if (event.type >= SDL_EVENT_WINDOW_FIRST && event.type <= SDL_EVENT_WINDOW_LAST)
		{
			CheckWindowState(event.window, &this->m_isActive);
		}
		break;
	}
	// this->m_windowRenderer->HandleEvent(&event);
}

Hush::WindowRenderer::~WindowRenderer()
{
	SDL_DestroyWindow(this->m_windowPtr);
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	SDL_Quit();
#endif
}

Hush::IRenderer *Hush::WindowRenderer::GetInternalRenderer() noexcept
{
	return nullptr;
	// return this->m_windowRenderer.get();
}

bool Hush::WindowRenderer::IsActive() const noexcept
{
	return this->m_isActive;
}

bool Hush::WindowRenderer::InitSDLIfNotStarted() noexcept
{
	if (SDL_WasInit(0) != 0)
	{
		return true;
	}
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	bool rc = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
	SDL_StartTextInput(m_windowPtr);
	return rc;
#else
	SDL_StartTextInput(m_windowPtr);
	return true;
#endif
}

void Hush::WindowRenderer::CheckWindowState(const SDL_WindowEvent windowEvent, bool *isActive) noexcept
{
	switch (windowEvent.type)
	{
	case SDL_EVENT_WINDOW_MINIMIZED:
		*isActive = false;
		break;
	case SDL_EVENT_WINDOW_RESTORED:
		*isActive = true;
		break;
	case SDL_EVENT_WINDOW_RESIZED:
		Hush::LogInfo("Window resized");
		this->m_windowRenderer->Resize(windowEvent.data1, windowEvent.data2);
		// Mark the render graph as dirty so it is fully rebuilt next frame
		// (transient resource dimensions depend on window size).  Unlike Reset(),
		// Invalidate() does not clear graph data mid-frame — the slow path in
		// RenderGraphSystem::OnPreRender() will handle the full Reset + rebuild.
		this->m_renderDevice->Invalidate();
		break;
	}
}
