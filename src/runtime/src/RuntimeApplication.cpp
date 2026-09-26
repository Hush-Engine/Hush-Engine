/*! \file RuntimeApplication.cpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Generic runtime player for exported Hush projects
*/

#include "RuntimeApplication.hpp"

#include "HushEngine.hpp"
#define HUSH_STATIC_BINDING
#include "HushBindings.h"
#undef HUSH_STATIC_BINDING
#include "LibManager.hpp"
#include "Logger.hpp"
#include "NativeModuleLoader.hpp"
#include "PakFileSystem.hpp"
#include "Scene.hpp"
#include "VirtualFilesystem.hpp"

#include <crypto/Hashing.hpp>

#include <cstdlib>

namespace
{
	std::filesystem::path PathFromUtf8(std::string_view value)
	{
		const auto *begin = reinterpret_cast<const char8_t *>(value.data());
		return std::filesystem::path(std::u8string(begin, begin + value.size()));
	}
} // namespace

Hush::RuntimeApplication::RuntimeApplication(HushEngine *engine)
	: m_engine(engine),
	  m_scene(std::make_unique<Scene>(engine, engine->GetEngineThreadPool()))
{
}

void Hush::RuntimeApplication::Init()
{
	// The manifest lives next to the executable, unless an override is given.
	std::filesystem::path manifestPath = LibManager::GetCurrentExecutablePath() / std::string(DEFAULT_MANIFEST_NAME);
#ifdef _WIN32
	wchar_t *manifestOverrideBuffer = nullptr;
	size_t manifestOverrideSize = 0;
	if (_wdupenv_s(&manifestOverrideBuffer, &manifestOverrideSize, L"HUSH_RUNTIME_MANIFEST") == 0 &&
		manifestOverrideBuffer != nullptr)
	{
		manifestPath = std::filesystem::path(manifestOverrideBuffer);
	}
	free(manifestOverrideBuffer);
#else
	if (const char *manifestOverride = std::getenv("HUSH_RUNTIME_MANIFEST"); manifestOverride != nullptr)
	{
		manifestPath = std::filesystem::path(manifestOverride);
	}
#endif

	Result<RuntimeManifest, RuntimeManifest::EError> manifestResult = RuntimeManifest::LoadFromFile(manifestPath);
	if (manifestResult.has_error())
	{
		LogFormat(ELogLevel::Error, "Could not load the runtime manifest at {}, error {}",
				  manifestPath.generic_string(), static_cast<int>(manifestResult.error()));
		return;
	}

	const RuntimeManifest &manifest = manifestResult.value();
	m_appName = manifest.name.empty() ? m_appName : manifest.name;
	const std::filesystem::path manifestDir = manifestPath.parent_path();

	// Mount the cooked content.
	if (!manifest.content.empty())
	{
		const std::filesystem::path contentPath = manifestDir / PathFromUtf8(manifest.content);
		if (!std::filesystem::exists(contentPath))
		{
			LogFormat(ELogLevel::Error, "Content pak file not found at {}", contentPath.generic_string());
		}
		else
		{
			m_engine->GetVirtualFilesystem()->MountFileSystem<PakFileSystem>("res://", contentPath);
		}
	}

	LoadModules(manifest, manifestDir);
	CreateSystems(manifest);

	if (!manifest.startupScene.empty())
	{
		// TODO: load the startup scene once the engine has a scene loader.
		LogFormat(ELogLevel::Info, "Startup scene {} is set, but scene loading is not implemented yet",
				  manifest.startupScene);
	}

	m_scene->Init();
}

void Hush::RuntimeApplication::LoadModules(const RuntimeManifest &manifest, const std::filesystem::path &manifestDir)
{
	Modules::NativeModuleLoader nativeLoader(*m_engine->GetModuleRegistry(), m_engine, &HUSH_FUNCPTR_TABLE);

	for (const RuntimeModuleManifest &module : manifest.modules)
	{
		if (module.kind == "dotnet-coreclr")
		{
			// TODO: load CoreCLR modules once the .NET host exists.
			LogFormat(ELogLevel::Error, "Module {} is a dotnet-coreclr module, which is not supported yet",
					  module.name);
			continue;
		}
		Modules::EModuleKind kind = Modules::EModuleKind::NativeCpp;
		if (module.kind == "rust")
		{
			kind = Modules::EModuleKind::Rust;
		}
		else if (module.kind == "dotnet-nativeaot")
		{
			kind = Modules::EModuleKind::DotNetNativeAot;
		}
		else if (module.kind != "native-module")
		{
			LogFormat(ELogLevel::Error, "Module {} has unsupported kind {}", module.name, module.kind);
			continue;
		}

		// native-module, rust and dotnet-nativeaot are all native libraries
		// with the same entry point.
		const std::filesystem::path modulePath = manifestDir / PathFromUtf8(module.path);
		Result<ModuleHandle, Modules::NativeModuleLoader::EError> loaded =
			nativeLoader.Load(modulePath, module.name, kind);
		if (loaded.has_error())
		{
			LogFormat(ELogLevel::Error, "Could not load module {} from {}, error {}", module.name,
					  modulePath.generic_string(), static_cast<int>(loaded.error()));
		}
	}
}

void Hush::RuntimeApplication::CreateSystems(const RuntimeManifest &manifest)
{
	Modules::ModuleRegistry *registry = m_engine->GetModuleRegistry();

	for (const RuntimeSystemManifest &system : manifest.systems)
	{
		const Reflection::TypeId typeId{Hashing::Fnv1a64(system.type)};

		// When the manifest does not name a module, search every loaded one.
		Result<ModuleHandle, Modules::ModuleRegistry::EError> module =
			system.module.empty() ? registry->FindSystemModule(typeId) : registry->FindModuleByName(system.module);

		if (module.has_error())
		{
			LogFormat(ELogLevel::Error, "Could not find a module for system {}", system.type);
			continue;
		}

		if (registry->AddSystemToScene(module.value(), typeId, *m_scene) != Modules::ModuleRegistry::EError::None)
		{
			LogFormat(ELogLevel::Error, "Could not create system {}", system.type);
		}
	}
}

void Hush::RuntimeApplication::Update(float delta)
{
	m_scene->Update(delta);
}

void Hush::RuntimeApplication::FixedUpdate(float delta)
{
	m_scene->FixedUpdate(delta);
}

void Hush::RuntimeApplication::OnPreRender()
{
	m_scene->PreRender();
}

void Hush::RuntimeApplication::OnRender(float)
{
	m_scene->Render();
}

void Hush::RuntimeApplication::OnPostRender()
{
	m_scene->PostRender();
}

void Hush::RuntimeApplication::DisposeFrame()
{
}

Hush::Scene *Hush::RuntimeApplication::GetScene()
{
	return m_scene.get();
}

std::string_view Hush::RuntimeApplication::GetAppName() const noexcept
{
	return m_appName;
}
