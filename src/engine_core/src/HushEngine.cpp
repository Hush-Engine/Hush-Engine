#include "HushEngine.hpp"
#include "ApplicationLoader.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "Logger.hpp"
#include "Shared/DirectionalLight.hpp"
#include <WindowManager.hpp>
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <imgui/imgui.h>

Hush::HushEngine::~HushEngine()
{
	this->Quit();
}

void Hush::HushEngine::Run()
{
	this->m_app = LoadApplication(this);

	this->m_isApplicationRunning = true;
	this->m_windowRenderer =
		std::make_unique<WindowRenderer>(this->m_app->GetAppName().data(), this->m_app->GetScene());

	// NOTE: BeginFrame/EndFrame are now managed by RenderGraphSystem
	// (OnPreRender and OnPostRender respectively). The graphics device
	// pointer is kept for any non-frame-lifecycle uses that may arise.
	[[maybe_unused]]
	auto *graphicsDevice = this->m_windowRenderer->GetGraphicsDevice();

	// Initialize any static resources we need
	this->Init();

	std::chrono::steady_clock::duration elapsed;

	while (this->m_isApplicationRunning)
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		m_windowRenderer->HandleEvents(&this->m_isApplicationRunning);
		// TODO: Change this to the window renderer
		if (!m_windowRenderer->IsActive())
		{
			// Avoid taking all CPU usage
			constexpr int32_t arbitrarySleepMs = 100;
			std::this_thread::sleep_for(std::chrono::milliseconds(arbitrarySleepMs));
			continue;
		}

		const float deltaTime = std::chrono::duration<float>(elapsed).count();

		this->m_app->Update(deltaTime);

		this->m_app->OnPreRender();

		this->m_app->OnRender(deltaTime);

		this->m_app->OnPostRender();

		this->m_app->DisposeFrame();
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		elapsed = end - start;
	}
}

Hush::HushEngine::HushEngine()
	: m_threadPool(Hush::Threading::Executors::ThreadPool::Create(
		  {.numThreads = std::thread::hardware_concurrency(), .pinToCore = true}))
{
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

extern "C" bool BundledAppExists_Internal_() HUSH_WEAK;

extern "C" Hush::IApplication *BundledApp_Internal_(Hush::HushEngine *engine) HUSH_WEAK;

std::unique_ptr<Hush::IApplication> Hush::LoadApplication(HushEngine *engine)
{
	// First, check if platform supports shared library app. If not, just attempt to load the bundled app.
#if !HUSH_SUPPORTS_SHARED_APP
	return BundledApp__Internal();
#else
	// Ok, we support apps as shared libraries, we then must check if a bundled application exists.
	if (BundledAppExists_Internal_())
	{
		// It exists, just return it.
		return std::unique_ptr<IApplication>(BundledApp_Internal_(engine));
	}

	// We can't find it, attempt to load it through a shared library.
	// TODO: define file metadata????

	return nullptr;
#endif
}
