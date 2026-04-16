#include "HushEngine.hpp"
#include "ApplicationLoader.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "Logger.hpp"
#include "ResourceManager.hpp"
#include "Shared/DirectionalLight.hpp"
#include "VirtualFilesystem.hpp"
#include "Systems/RenderGraphSystem.hpp"
#include "Systems/ResourceUploadSystem.hpp"
#include "WindowRenderer.hpp"
#include "filesystem/CFileSystem/CFileSystem.hpp"
#include <WindowManager.hpp>
#include <algorithm>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include "Profiling.hpp"
#include <glm/trigonometric.hpp>
#include <imgui/imgui.h>

struct Hush::HushEngine::HushEngineInternal
{
	Hush::VirtualFilesystem vfs;
	Hush::ResourceManager resourceManager;

	std::unique_ptr<Graphics::RenderGraphSystem> renderGraphSystem;
	std::unique_ptr<Hush::Renderer::ResourceUploadSystem> resourceUploadSystem;
	std::unique_ptr<WindowRenderer> windowRenderer = nullptr;
};

Hush::HushEngine::HushEngine()
	: m_threadPool(Hush::Threading::Executors::ThreadPool::Create(
		  {.numThreads = std::thread::hardware_concurrency(), .pinToCore = true}))
{
	m_internal = std::make_unique<HushEngineInternal>();
	m_internal->resourceManager.Init(&m_internal->vfs);
}

Hush::HushEngine::~HushEngine()
{
	this->Quit();
}

void Hush::HushEngine::Run(std::span<const char *> args)
{
	// Load the VFS with the default data directory (this is where the engine looks for assets by default, but users can
	// mount additional directories or archives as needed)
	this->m_internal->vfs.MountFileSystem<Hush::CFileSystem>("engine_res://", "./");

	this->m_app = LoadApplication(this);

	// Check for --wait-profiler flag
	if (std::ranges::find_if(args, [](const char *arg) { return std::string_view(arg) == "--wait-profiler"; }) !=
		args.end())
	{
		LogInfo("Waiting for Tracy profiler to connect...");
		while (!TracyIsConnected)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
		LogInfo("Tracy profiler connected!");
	}

	this->m_isApplicationRunning = true;
	this->m_internal->windowRenderer =
		std::make_unique<WindowRenderer>(this->m_app->GetAppName().data(), this->m_app->GetScene());

	this->m_internal->renderGraphSystem = std::make_unique<Graphics::RenderGraphSystem>(
		*this->m_app->GetScene(), &this->m_internal->windowRenderer->GetRenderDevice());

	this->m_internal->resourceUploadSystem = std::make_unique<Hush::Renderer::ResourceUploadSystem>(
		*this->m_app->GetScene(), this->m_internal->renderGraphSystem->GetRenderDevice());

	AddDefaultSystems();

	// Initialize any static resources we need
	this->Init();

	std::chrono::steady_clock::duration elapsed;

	while (this->m_isApplicationRunning)
	{
		ZoneScoped;
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

		{
			ZoneScopedN("HandleEvents");
			this->m_internal->windowRenderer->HandleEvents(&this->m_isApplicationRunning);
		}

		// TODO: Change this to the window renderer
		if (!this->m_internal->windowRenderer->IsActive())
		{
			// Avoid taking all CPU usage
			constexpr int32_t arbitrarySleepMs = 100;
			std::this_thread::sleep_for(std::chrono::milliseconds(arbitrarySleepMs));
			continue;
		}

		const float deltaTime = std::chrono::duration<float>(elapsed).count();

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

		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		elapsed = end - start;
		FrameMark;
	}
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

void Hush::HushEngine::Init()
{
	this->m_app->Init();
	// Add a default directional light
	Scene *scene = this->m_app->GetScene();
	Entity entity = scene->CreateEntityWithName("Directional Light");
	WorldTransform &transform = entity.AddComponent<WorldTransform>();
	transform.SetEulerAngles(glm::radians(glm::vec3(-45.0F, 0.0F, 0.0F)));
	entity.AddComponent<LocalTransform>();
	this->m_defaultLight = &entity.AddComponent<DirectionalLight>();
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
	this->m_app->GetScene()->AddEngineSystem(this->m_internal->renderGraphSystem.get());
	this->m_app->GetScene()->AddEngineSystem(this->m_internal->resourceUploadSystem.get());
}

Hush::ResourceManager *Hush::HushEngine::GetResourceManager() noexcept
{
	return &this->m_internal->resourceManager;
}
