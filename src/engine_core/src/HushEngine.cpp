#include "HushEngine.hpp"
#include "ApplicationLoader.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
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
	WindowRenderer mainRenderer(m_app->GetAppName().data(), this->m_app->GetScene());
	IRenderer *rendererImpl = mainRenderer.GetInternalRenderer();

	// Initialize any static resources we need
	this->Init();
	rendererImpl->SetActiveScene(this->m_app->GetScene());

	std::chrono::steady_clock::duration elapsed;

	while (this->m_isApplicationRunning)
	{
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		mainRenderer.HandleEvents(&this->m_isApplicationRunning);
		// TODO: Change this to the window renderer
		if (!mainRenderer.IsActive())
		{
			// Avoid taking all CPU usage
			constexpr int32_t arbitrarySleepMs = 100;
			std::this_thread::sleep_for(std::chrono::milliseconds(arbitrarySleepMs));
			continue;
		}

		const float deltaTime = std::chrono::duration<float>(elapsed).count();

		this->m_app->Update(deltaTime);

		this->m_app->OnPreRender();

		rendererImpl->NewUIFrame();

		this->m_app->OnRender();

		rendererImpl->Draw(deltaTime);

		this->m_app->OnPostRender();

		this->m_app->DisposeFrame();
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		elapsed = end - start;
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
