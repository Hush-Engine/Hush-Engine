#include "HushEngine.hpp"
#include "ApplicationLoader.hpp"
#include "Logger.hpp"
#include "ResourceManager.hpp"
#include "Scene.hpp"
#include "VirtualFilesystem.hpp"
#include "Systems/RenderGraphSystem.hpp"
#include "Systems/ResourceUploadSystem.hpp"
#include "WindowRenderer.hpp"
#include "Hush/Memory/ThreadLocalMemoryResourcePool.hpp"
#include "filesystem/CFileSystem/CFileSystem.hpp"
#include <SDL3/SDL_keyboard.h>
#include <WindowManager.hpp>
#include <algorithm>
#include <cstdint>
#include "Profiling.hpp"
#include <imgui/imgui.h>
#include "Platform.hpp"

#if defined(HUSH_USE_MIMALLOC)
#include <mimalloc.h>
extern "C" void HushForceLinkAllocatorOverrides() noexcept;
#elif defined(HUSH_ENABLE_PROFILING) && HUSH_PLATFORM_WIN
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif
#include <vector>

// Per-thread initial buffer size, in KB, for the frame and scene arenas.
static constexpr std::size_t HUSH_FRAME_ARENA_SIZE_KB = 256;
static constexpr std::size_t HUSH_SCENE_ARENA_SIZE_KB = 256;

struct Hush::HushEngine::HushEngineInternal
{
	Hush::VirtualFilesystem vfs;
	Hush::ResourceManager resourceManager;

	std::unique_ptr<Graphics::RenderGraphSystem> renderGraphSystem;
	std::unique_ptr<Hush::Renderer::ResourceUploadSystem> resourceUploadSystem;
	std::unique_ptr<WindowRenderer> windowRenderer = nullptr;
	Hush::Memory::ThreadLocalMemoryResourcePool frameMemoryPool{HUSH_FRAME_ARENA_SIZE_KB * 1024};
	Hush::Memory::ThreadLocalMemoryResourcePool sceneMemoryPool{HUSH_SCENE_ARENA_SIZE_KB * 1024};
};

#if defined(HUSH_PLATFORM_EMSCRIPTEN)
static constexpr uint32_t NUM_THREADS = 4;
#else
static constexpr uint32_t NUM_THREADS = std::thread::hardware_concurrency();
#endif

Hush::HushEngine::HushEngine()
	: m_threadPool(Hush::Threading::Executors::ThreadPool::Create({.numThreads = NUM_THREADS, .pinToCore = true}))
{
#if defined(HUSH_USE_MIMALLOC)
	HushForceLinkAllocatorOverrides();
#endif
	m_internal = std::make_unique<HushEngineInternal>();
	m_internal->resourceManager.Init(&m_internal->vfs);
}

Hush::HushEngine::~HushEngine()
{
	this->Quit();
}

void Hush::HushEngine::Init(int argc, char **argv)
{
#ifndef HUSH_ENGINE_RES_DIR
#if HUSH_PLATFORM_EMSCRIPTEN
	constexpr std::string_view engineResDir = "/";
#else
	constexpr std::string_view engineResDir = "./";
#endif
#else
	constexpr std::string_view engineResDir = HUSH_ENGINE_RES_DIR;
#endif
	// Load the VFS with the default data directory
	this->m_internal->vfs.MountFileSystem<Hush::CFileSystem>("engine_res://", engineResDir);

	this->m_app = LoadApplication(this);

	// Wire the engine-owned frame/scene memory resources into the scene, so it can cheaply
	// materialize null-terminated strings for Flecs and allocate scene-lifetime data from the
	// scene arena.
	this->m_app->GetScene()->SetMemoryResources(&this->m_internal->frameMemoryPool, &this->m_internal->sceneMemoryPool);

	// Check for --wait-profiler flag
	std::span<char *> args(argv, static_cast<size_t>(argc));
	if (std::ranges::find_if(args, [](const char *arg) { return std::string_view(arg) == "--wait-profiler"; }) !=
		args.end())
	{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
		LogInfo("Waiting for Tracy profiler to connect...");
		while (!TracyIsConnected)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		LogInfo("Tracy profiler connected!");
#endif
	}

	this->m_isApplicationRunning = true;
	this->m_internal->windowRenderer =
		std::make_unique<WindowRenderer>(this->m_app->GetAppName().data(), this->m_app->GetScene());

	this->m_internal->renderGraphSystem = std::make_unique<Graphics::RenderGraphSystem>(
		*this->m_app->GetScene(), &this->m_internal->windowRenderer->GetRenderDevice());

	this->m_internal->resourceUploadSystem = std::make_unique<Hush::Renderer::ResourceUploadSystem>(
		*this->m_app->GetScene(), this->m_internal->renderGraphSystem->GetRenderDevice());

	AddDefaultSystems();

	this->m_app->Init();
}

void Hush::HushEngine::Run()
{
	std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

	if (!this->m_internal->windowRenderer->IsActive())
	{
		// Avoid taking all CPU usage
		constexpr int32_t arbitrarySleepMs = 100;
		std::this_thread::sleep_for(std::chrono::milliseconds(arbitrarySleepMs));
		return;
	}

#ifndef HUSH_PLATFORM_EMSCRIPTEN
	ZoneScoped;
#endif

	const float deltaTime = std::chrono::duration<float>(m_elapsed).count();

	this->m_app->Update(deltaTime);
	this->m_app->OnPreRender();
	this->m_app->OnRender(deltaTime);
	this->m_app->OnPostRender();
	this->m_app->DisposeFrame();

	// ImGUI's SDL3 impl will sometimes disable text input for some reason, this is here to counter that
	SDL_StartTextInput(this->GetWindowRenderer()->GetSDLWindow());
	this->m_internal->frameMemoryPool.Reset();

	// Publish this frame's frame-arena usage to Tracy (no-op when profiling is off). The plots
	// show whether the configured arena size is right: "Spill" > 0 means the buffer was too small.
	const Hush::Memory::ThreadLocalMemoryResourcePool::Stats frameStats = this->m_internal->frameMemoryPool.GetStats();
	TracyPlot("Frame Arena Bytes", static_cast<double>(frameStats.bytesRequestedLastCycle));
	TracyPlot("Frame Arena Spill", static_cast<double>(frameStats.spilledBytesLastCycle));

	// The scene-scoped pool is intentionally NOT reset here: its allocations persist across
	// frames and are reclaimed on scene teardown via ResetSceneScopeMemory().

	InputManager::ResetMouseAcceleration();
	InputManager::ResetCharData();

	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	m_elapsed = end - start;
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	FrameMark;
#endif
}

void Hush::HushEngine::HandleEvents(const SDL_Event &event)
{
	this->m_internal->windowRenderer->HandleEvents(&this->m_isApplicationRunning, event);
}

void Hush::HushEngine::AddSystem(ISystem *system)
{
	this->m_app->GetScene()->AddEngineSystem(system);
}

void Hush::HushEngine::Quit()
{
	this->m_isApplicationRunning = false;
}

Hush::Scene *Hush::HushEngine::GetScene()
{
	return this->m_app->GetScene();
}

Hush::Scene* Hush::HushEngine::NewScene() {
	// NYI: scene
	return nullptr;
}

[[hush::export]]
Hush::HushEngine::EError Hush::HushEngine::LoadScene(Scene* scene) {
	// NYI: Scene
	(void)scene;
	return EError::None;
}

Hush::WindowRenderer *Hush::HushEngine::GetWindowRenderer() noexcept
{
	return this->m_internal->windowRenderer.get();
}

Hush::VirtualFilesystem *Hush::HushEngine::GetVirtualFilesystem() noexcept
{
	return &this->m_internal->vfs;
}

void Hush::HushEngine::AddDefaultSystems()
{
	this->m_app->GetScene()->AddEngineSystem(this->m_internal->resourceUploadSystem.get());
	this->m_app->GetScene()->AddEngineSystem(this->m_internal->renderGraphSystem.get());
}

Hush::ResourceManager *Hush::HushEngine::GetResourceManager() noexcept
{
	return &this->m_internal->resourceManager;
}

std::pmr::memory_resource *Hush::HushEngine::GetFrameScopeMemoryResource() noexcept
{
	return &this->m_internal->frameMemoryPool;
}

std::pmr::memory_resource *Hush::HushEngine::GetSceneScopeAllocator() noexcept
{
	return &this->m_internal->sceneMemoryPool;
}

void Hush::HushEngine::ResetSceneScopeMemory() noexcept
{
	this->m_internal->sceneMemoryPool.Reset();
}
