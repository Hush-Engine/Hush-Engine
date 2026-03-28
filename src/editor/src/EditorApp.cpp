
//
// Created by Alan5 on 22/09/2024.
//

#include "Assertions.hpp"
#include "HushEngine.hpp"
#include "IApplication.hpp"
#include "ISystem.hpp"
#include "Scene.hpp"
#include "TransformationSystem.hpp"
#include "UI.hpp"
#include "VirtualFilesystem.hpp"
#include "components/EditorInfo.hpp"
#include "ResourceManager.hpp"
#include "filesystem/CFileSystem/CFileSystem.hpp"
#include "ScriptingHost.hpp"
#include "systems/EditorCameraSystem.hpp"
#include "systems/RenderingSystem.hpp"
#include <cstdint>
#include <memory>
#include <vector>

// This is temporary lol
#if __has_include("../../bindings/HushBindings.cpp")
#define HUSH_STATIC_BINDING
#include "../../bindings/HushBindings.cpp"
#endif

class EditorApp final : public Hush::IApplication
{
public:
	EditorApp(Hush::HushEngine *engine)
		: m_scene(std::make_unique<Hush::Scene>(engine, engine->GetEngineThreadPool()))
	{
	}

	EditorApp(const EditorApp &) = delete;
	EditorApp(EditorApp &&) = delete;
	EditorApp &operator=(const EditorApp &) = delete;
	EditorApp &operator=(EditorApp &&) = delete;

	~EditorApp() override = default;

	void Init() override
	{
		this->m_scriptingHost.Initialize("C:/Users/nefes/Personal/HushBindingGen/build/Debug_Win64/beef-hush/beef-hush.dll");
		this->m_scriptingHost.GetStartScriptingConnectionFn()(&HUSH_FUNCPTR_TABLE, this->m_scene->GetEngine());
		std::vector<Hush::ScriptingSystemInfo>& systems = this->m_scriptingHost.GetAvailableSystems();
		Hush::ScriptingSystemInfo systemInfo = systems.at(0);
		auto foundRes = this->m_scriptingHost.CreateSystem(systemInfo);
		HUSH_RESULT_ASSERT(foundRes, "Unable to instantiate SmallSystem");
		this->m_testSystem = foundRes.value();
		
		this->m_scriptingHost.GetCallSystemInitFn()(reinterpret_cast<void*>(this->m_testSystem));

		// Make the giant System pool
		this->m_cameraSystem = std::make_unique<Hush::EditorCameraSystem>(*this->m_scene);
		this->m_scene->AddEngineSystem(new Hush::RenderingSystem(*this->m_scene));
		this->m_scene->AddEngineSystem(new Hush::TransformationSystem(*this->m_scene));
		this->m_scene->AddEngineSystem(this->m_cameraSystem.get());
		Hush::Entity entt = this->m_scene->CreateEntityWithName("EngineManager");
		entt.AddComponent<Hush::EditorInfo>();
		this->m_resourceManager = &entt.AddComponent<Hush::ResourceManager>();
		Hush::VirtualFilesystem &vfs = entt.AddComponent<Hush::VirtualFilesystem>();
		vfs.MountFileSystem<Hush::CFileSystem>("res://", HUSH_DEFAULT_PROJECT_DIR);
		vfs.MountFileSystem<Hush::CFileSystem>("engine_res://", "./");
		this->m_scene->Init();
		this->m_userInterface.Init(this->m_scene.get());
	}

	void Update(float delta) override
	{
		this->m_scene->Update(delta);
		this->m_scriptingHost.GetCallSystemOnUpdateFn()(reinterpret_cast<void*>(this->m_testSystem), 0.16);
	}

	void FixedUpdate(float delta) override
	{
		this->m_scene->FixedUpdate(delta);
	}

	void OnRender(float delta) override
	{
		this->m_scene->Render();
		this->m_userInterface.DrawPanels(delta);
	}

	void OnPostRender() override
	{
		this->m_scene->PostRender();
	}

	void OnPreRender() override
	{
		this->m_scene->PreRender();
	}

	void DisposeFrame() override
	{
		this->m_resourceManager->FreePending();
	}

	[[nodiscard]]
	std::string_view GetAppName() const noexcept override
	{
		return "Hush-Editor";
	}

	Hush::Scene *GetScene() noexcept override
	{
		return this->m_scene.get();
	}

private:
	Hush::UI m_userInterface;
	Hush::ResourceManager *m_resourceManager = nullptr;
	std::unique_ptr<Hush::Scene> m_scene;
	std::unique_ptr<Hush::EditorCameraSystem> m_cameraSystem;
	Hush::ScriptingHost m_scriptingHost;
	uintptr_t m_testSystem = 0;
};

extern "C" bool BundledAppExists_Internal_() // NOLINT(*-identifier-naming)
{
	return true;
}

extern "C" Hush::IApplication *BundledApp_Internal_(Hush::HushEngine *engine) // NOLINT(*-identifier-naming)
{
	return new EditorApp(engine);
}
