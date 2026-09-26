/*! \file NativeModuleLoader.cpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Loads native gameplay modules from dynamic libraries
*/

#include "NativeModuleLoader.hpp"

#include "Logger.hpp"
#include "NativeModuleEntry.hpp"
#include "SharedLibrary.hpp"

#include <filesystem>
#include <type_traits>

Hush::Modules::NativeModuleLoader::NativeModuleLoader(ModuleRegistry &registry, HushEngine *engine,
													  const HushFuncPtrTable *hushApi)
	: m_registry(registry),
	  m_engine(engine),
	  m_hushApi(hushApi)
{
}

Hush::Result<Hush::ModuleHandle, Hush::Modules::NativeModuleLoader::EError> Hush::Modules::NativeModuleLoader::Load(
	const std::filesystem::path &path, std::string_view moduleName, EModuleKind kind)
{
	Result<SharedLibrary, SharedLibrary::EError> libraryResult = SharedLibrary::OpenSharedLibrary(path);
	if (libraryResult.has_error())
	{
		LogFormat(ELogLevel::Error, "Could not load module library at {}", path.generic_string());
		return EError::LibraryLoadFailed;
	}
	SharedLibrary library = std::move(libraryResult.value());

	auto entryPoint = library.GetSymbolUnsafe<std::remove_pointer_t<HushRegisterModuleFn>>(HUSH_REGISTER_MODULE_NAME);
	if (entryPoint == nullptr)
	{
		LogFormat(ELogLevel::Error, "Module at {} does not export {}", path.generic_string(),
				  HUSH_REGISTER_MODULE_NAME);
		return EError::EntryPointNotFound;
	}

	// The module handle is created before calling the entry point so every
	// type the module registers is owned by this module from the start.
	const std::u8string pathStem = path.stem().u8string();
	std::string resolvedModuleName =
		moduleName.empty() ? std::string(pathStem.begin(), pathStem.end()) : std::string(moduleName);
	Result<ModuleHandle, ModuleRegistry::EError> module =
		m_registry.BeginModuleRegistration(resolvedModuleName, kind, HUSH_MODULE_ABI_VERSION);
	if (module.has_error())
	{
		return EError::RegistrationFailed;
	}

	NativeModuleHost host;
	host.engine = m_engine;
	host.reflectionDB = &m_registry.GetReflectionDB();
	host.moduleRegistry = &m_registry;

	HushModuleContext context;
	context.structSize = sizeof(HushModuleContext);
	context.abiVersion = HUSH_MODULE_ABI_VERSION;
	context.engine = m_engine;
	context.module = module.value();
	context.hushApi = m_hushApi;
	context.hostData = &host;

	const HushModuleResult result = entryPoint(&context);
	if (result == HushModuleResult_AbiVersionMismatch)
	{
		LogFormat(ELogLevel::Error, "Module {} was built for a different module ABI version", path.generic_string());
		m_registry.AbortModuleRegistration(module.value());
		return EError::AbiVersionMismatch;
	}
	if (result != HushModuleResult_Ok)
	{
		LogFormat(ELogLevel::Error, "Module {} failed to register, error {}", path.generic_string(),
				  static_cast<int>(result));
		m_registry.AbortModuleRegistration(module.value());
		return EError::RegistrationFailed;
	}

	if (m_registry.SetLibrary(module.value(), std::move(library)) != ModuleRegistry::EError::None)
	{
		m_registry.AbortModuleRegistration(module.value());
		return EError::RegistrationFailed;
	}
	if (m_registry.CommitModuleRegistration(module.value()) != ModuleRegistry::EError::None)
	{
		m_registry.AbortModuleRegistration(module.value());
		return EError::RegistrationFailed;
	}
	return module.value();
}
