#include "ScriptingHost.hpp"
#include "Assertions.hpp"
#include "LibManager.hpp"
#include "Logger.hpp"
#include "Scene.hpp"
#include "VirtualFilesystem.hpp"
#include <cstdint>

constexpr std::string_view START_SCRIPTING_CONNECTION = "StartScriptingConnection";
constexpr std::string_view DISPOSE_SCRIPTING_CONNECTION = "DisposeScriptingConnection";

constexpr std::string_view GET_AVAILABLE_SYSTEMS_FN_NAME = "GetAvailableSystems";
constexpr std::string_view GET_SYSTEM_COUNT_FN_NAME = "GetSystemCount";

constexpr std::string_view GET_AVAILABLE_COMPONENTS_FN_NAME = "GetAvailableComponents";
constexpr std::string_view GET_COMPONENT_COUNT_FN_NAME = "GetComponentCount";

constexpr std::string_view INSTANTIATE_SYSTEM_FN_NAME = "InstantiateSystem";
constexpr std::string_view CALL_SYSTEM_INIT_FN_NAME = "CallSystemInit";
constexpr std::string_view CALL_SYSTEM_ON_UPDATE_FN_NAME = "CallSystemOnUpdate";
constexpr std::string_view CALL_SYSTEM_ON_FIXEDUPDATE_FN_NAME = "CallSystemOnFixedUpdate";
constexpr std::string_view CALL_SYSTEM_ON_SHUTDOWN_FN_NAME = "CallSystemOnShutdown";
constexpr std::string_view CALL_SYSTEM_ON_RENDER_FN_NAME = "CallSystemOnRender";
constexpr std::string_view CALL_SYSTEM_ON_PRERENDER_FN_NAME = "CallSystemOnPreRender";
constexpr std::string_view CALL_SYSTEM_ON_POSTRENDER_FN_NAME = "CallSystemOnPostRender";

#define BIND_DLL_SCRIPTING_FUNCTION(fnName, functionPtr)                                                               \
	do                                                                                                                 \
	{                                                                                                                  \
		void *_fnPtrAddress = LibManager::DynamicLoadSymbol(libraryHandle, fnName.data());                             \
		HUSH_ASSERT(_fnPtrAddress != nullptr, "Could not load function for symbol {}", fnName);                        \
		(functionPtr) = reinterpret_cast<decltype(functionPtr)>(_fnPtrAddress);                                        \
	} while (0)

// NOLINTBEGIN
void Hush::ScriptingHost::Initialize(NullTerminatedStringView dllPath, VirtualFilesystem* vfs, Scene* scene)
{
	// TODO: reconcile with virtual filesystem
	auto res = vfs->ResolveHostPath(dllPath);
	HUSH_RESULT_ASSERT(res, "Could not resolve virtual path for scripting at: {}", dllPath);
	void *libraryHandle = LibManager::LibraryOpen(res.value().string().c_str());

	// This could be user side??? eventually
	HUSH_COND_FAIL_MSG(libraryHandle != nullptr, "Could not load dynamic library at {}", std::string_view(dllPath));

	BIND_DLL_SCRIPTING_FUNCTION(START_SCRIPTING_CONNECTION, this->m_startScriptingConnectionFn);
	BIND_DLL_SCRIPTING_FUNCTION(DISPOSE_SCRIPTING_CONNECTION, this->m_disposeScriptingConnectionFn);

	BIND_DLL_SCRIPTING_FUNCTION(GET_AVAILABLE_SYSTEMS_FN_NAME, this->m_getAvailableSystemsFn);
	BIND_DLL_SCRIPTING_FUNCTION(GET_SYSTEM_COUNT_FN_NAME, this->m_getSystemCountFn);

	BIND_DLL_SCRIPTING_FUNCTION(GET_AVAILABLE_COMPONENTS_FN_NAME, this->m_getAvailableComponentsFn);
	BIND_DLL_SCRIPTING_FUNCTION(GET_COMPONENT_COUNT_FN_NAME, this->m_getComponentCountFn);

	BIND_DLL_SCRIPTING_FUNCTION(INSTANTIATE_SYSTEM_FN_NAME, this->m_instantiateSystemFn);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_INIT_FN_NAME, this->m_scriptingInterface.initFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_UPDATE_FN_NAME, this->m_scriptingInterface.updateFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_FIXEDUPDATE_FN_NAME, this->m_scriptingInterface.fixedUpdateFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_SHUTDOWN_FN_NAME, this->m_scriptingInterface.shutdownFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_RENDER_FN_NAME, this->m_scriptingInterface.renderFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_PRERENDER_FN_NAME, this->m_scriptingInterface.preRenderFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_POSTRENDER_FN_NAME, this->m_scriptingInterface.postRenderFunction);

	// HACK: This should probably live somewhere else
	// Update the cache
	std::vector<Hush::ScriptingRegisteredTypeInfo>& comps = this->GetAvailableComponents();

	for (const ScriptingRegisteredTypeInfo& typeInfo : comps) {
		auto nameView = std::string_view(static_cast<const char *>(typeInfo.name));

		// Register the component
		(void)scene->RegisterComponentRaw({
			.size = typeInfo.byteSize,
			.alignment = typeInfo.align,
			.name = nameView.data(),
		});
		
	}
	
}
// NOLINTEND

std::vector<Hush::ScriptingRegisteredTypeInfo> &Hush::ScriptingHost::GetAvailableSystems()
{
	if (m_libIsDirty)
	{
		this->FetchSystemsIntoCache();
		this->FetchComponentsIntoCache();
	}
	return this->m_availableSystems;
}

std::vector<Hush::ScriptingRegisteredTypeInfo> &Hush::ScriptingHost::GetAvailableComponents()
{
	if (m_libIsDirty)
	{
		this->FetchComponentsIntoCache();
		this->FetchSystemsIntoCache();
	}
	return this->m_availableComponents;
}

void Hush::ScriptingHost::FetchSystemsIntoCache()
{
	HUSH_ASSERT(this->m_getSystemCountFn != nullptr,
				"Function pointer to get system count is not initialized, forgot to call ScriptingHost::Initialize?");
	uint64_t sysCount = this->m_getSystemCountFn();
	Hush::LogFormat(ELogLevel::Info, "Got {} systems from the scripting system!", sysCount);
	this->m_availableSystems.resize(sysCount);

	HUSH_ASSERT(
		this->m_getAvailableSystemsFn != nullptr,
		"Function pointer to get available systems is not initialized, forgot to call ScriptingHost::Initialize?");
	ScriptingRegisteredTypeInfo *systemsArr = this->m_availableSystems.data();

	this->m_getAvailableSystemsFn(&systemsArr, this->m_availableSystems.size());
	this->m_libIsDirty = false;
}

void Hush::ScriptingHost::FetchComponentsIntoCache()
{
	HUSH_ASSERT(this->m_getComponentCountFn != nullptr,
				"Function pointer to get component count is not initialized, forgot to call ScriptingHost::Initialize?");
	uint64_t compCount = this->m_getComponentCountFn();
	Hush::LogFormat(ELogLevel::Info, "Got {} components from the scripting system!", compCount);
	this->m_availableComponents.resize(compCount);

	HUSH_ASSERT(
		this->m_getAvailableComponentsFn != nullptr,
		"Function pointer to get available components is not initialized, forgot to call ScriptingHost::Initialize?");
	ScriptingRegisteredTypeInfo *componentsArr = this->m_availableComponents.data();

	this->m_getAvailableComponentsFn(&componentsArr, this->m_availableComponents.size());
	this->m_libIsDirty = false;
}

Hush::Result<uintptr_t, Hush::ScriptingHost::EError> Hush::ScriptingHost::CreateSystem(
	const ScriptingRegisteredTypeInfo &systemInfo)
{
	HUSH_ASSERT(this->m_instantiateSystemFn != nullptr,
				"Function pointer to create systems is not initialized, forgot to call ScriptingHost::Initialize?");

	auto nameView = std::string_view(static_cast<const char *>(systemInfo.name));
	HUSH_COND_FAIL_V(!nameView.empty(), EError::SystemNotFound);
	uint8_t *sysPtr = this->m_instantiateSystemFn(&systemInfo);
	HUSH_COND_FAIL_MSG_V(sysPtr != nullptr, EError::SystemNotFound,
						 "Could not find system named {} in the scripting host", systemInfo.name);
	return reinterpret_cast<uintptr_t>(sysPtr);
}
