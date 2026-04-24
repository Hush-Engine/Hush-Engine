#include "ScriptingHost.hpp"
#include "Assertions.hpp"
#include "LibManager.hpp"
#include "Logger.hpp"
#include <cstdint>

constexpr std::string_view START_SCRIPTING_CONNECTION = "StartScriptingConnection";
constexpr std::string_view DISPOSE_SCRIPTING_CONNECTION = "DisposeScriptingConnection";

constexpr std::string_view GET_AVAILABLE_SYSTEMS_FN_NAME = "GetAvailableSystems";
constexpr std::string_view GET_SYSTEM_COUNT_FN_NAME = "GetSystemCount";

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
void Hush::ScriptingHost::Initialize(std::string_view dllPath)
{
	// TODO: reconcile with virtual filesystem
	void *libraryHandle = LibManager::LibraryOpen(dllPath.data());

	// This could be user side??? eventually
	HUSH_COND_FAIL_MSG(libraryHandle != nullptr, "Could not load dynamic library at {}", dllPath);

	BIND_DLL_SCRIPTING_FUNCTION(START_SCRIPTING_CONNECTION, this->m_startScriptingConnectionFn);
	BIND_DLL_SCRIPTING_FUNCTION(DISPOSE_SCRIPTING_CONNECTION, this->m_disposeScriptingConnectionFn);

	BIND_DLL_SCRIPTING_FUNCTION(GET_AVAILABLE_SYSTEMS_FN_NAME, this->m_getAvailableSystemsFn);
	BIND_DLL_SCRIPTING_FUNCTION(GET_SYSTEM_COUNT_FN_NAME, this->m_getSystemCountFn);

	BIND_DLL_SCRIPTING_FUNCTION(INSTANTIATE_SYSTEM_FN_NAME, this->m_instantiateSystemFn);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_INIT_FN_NAME, this->m_scriptingInterface.initFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_UPDATE_FN_NAME, this->m_scriptingInterface.updateFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_FIXEDUPDATE_FN_NAME, this->m_scriptingInterface.fixedUpdateFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_SHUTDOWN_FN_NAME, this->m_scriptingInterface.shutdownFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_RENDER_FN_NAME, this->m_scriptingInterface.renderFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_PRERENDER_FN_NAME, this->m_scriptingInterface.preRenderFunction);
	BIND_DLL_SCRIPTING_FUNCTION(CALL_SYSTEM_ON_POSTRENDER_FN_NAME, this->m_scriptingInterface.postRenderFunction);
}
// NOLINTEND

std::vector<Hush::ScriptingSystemInfo> &Hush::ScriptingHost::GetAvailableSystems()
{
	if (m_libIsDirty)
	{
		this->FetchSystemsIntoCache();
	}
	return this->m_availableSystems;
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
	ScriptingSystemInfo *systemsArr = this->m_availableSystems.data();

	this->m_getAvailableSystemsFn(&systemsArr, this->m_availableSystems.size());
	this->m_libIsDirty = false;
}

Hush::Result<uintptr_t, Hush::ScriptingHost::EError> Hush::ScriptingHost::CreateSystem(
	const ScriptingSystemInfo &systemInfo)
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
