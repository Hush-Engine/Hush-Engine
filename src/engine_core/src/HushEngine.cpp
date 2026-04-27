#include "HushEngine.hpp"
#include "ApplicationLoader.hpp"
#include "Logger.hpp"
#include "ResourceManager.hpp"
#include "VirtualFilesystem.hpp"
#include "Systems/RenderGraphSystem.hpp"
#include "Systems/ResourceUploadSystem.hpp"
#include "WindowRenderer.hpp"
#include "filesystem/CFileSystem/CFileSystem.hpp"
#include <WindowManager.hpp>
#include <algorithm>
#include <cstdint>
#include "Profiling.hpp"
#include <imgui/imgui.h>
#include <vector>

struct Hush::HushEngine::HushEngineInternal
{
	Hush::VirtualFilesystem vfs;
	Hush::ResourceManager resourceManager;

	std::unique_ptr<Graphics::RenderGraphSystem> renderGraphSystem;
	std::unique_ptr<Hush::Renderer::ResourceUploadSystem> resourceUploadSystem;
	std::unique_ptr<WindowRenderer> windowRenderer = nullptr;
};

#if defined(HUSH_PLATFORM_EMSCRIPTEN)
static constexpr uint32_t NUM_THREADS = 4;
#else
static constexpr uint32_t NUM_THREADS = std::thread::hardware_concurrency();
#endif

Hush::HushEngine::HushEngine()
	: m_threadPool(Hush::Threading::Executors::ThreadPool::Create(
		  {.numThreads = NUM_THREADS, .pinToCore = true}))
{
	m_internal = std::make_unique<HushEngineInternal>();
	m_internal->resourceManager.Init(&m_internal->vfs);
}

Hush::HushEngine::~HushEngine()
{
	this->Quit();
}

void Hush::HushEngine::Init(int argc, char **argv)
{
	// Load the VFS with the default data directory
#if HUSH_PLATFORM_EMSCRIPTEN
	this->m_internal->vfs.MountFileSystem<Hush::CFileSystem>("engine_res://", "/");
#else
	this->m_internal->vfs.MountFileSystem<Hush::CFileSystem>("engine_res://", "./");
#endif

	this->m_app = LoadApplication(this);

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

#ifndef HUSH_PLATFORM_EMSCRIPTEN
	{
		ZoneScopedN("Update");
		this->m_app->Update(deltaTime);
	}

	{
		ZoneScopedN("PreRender");
		this->m_app->OnPreRender();
	}

	{
		ZoneScopedN("Render");
		this->m_app->OnRender(deltaTime);
	}

	{
		ZoneScopedN("PostRender");
		this->m_app->OnPostRender();
	}

	{
		ZoneScopedN("DisposeFrame");
		this->m_app->DisposeFrame();
	}
#else
	this->m_app->Update(deltaTime);
	this->m_app->OnPreRender();
	this->m_app->OnRender(deltaTime);
	this->m_app->OnPostRender();
	this->m_app->DisposeFrame();
#endif

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
