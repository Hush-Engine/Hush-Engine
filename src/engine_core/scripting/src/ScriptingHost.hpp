#pragma once

#include "Assertions.hpp"
#include "NullTerminatedStringView.hpp"
#include "Result.hpp"
#include <string_view>
#include <vector>
#include <ISystem.hpp>

// fwd declaration
struct HushFuncPtrTable;

namespace Hush
{

	class HushEngine;

	struct ScriptingSystemInfo
	{
		static constexpr int32_t MAX_SYS_NAME = 64;
		char name[MAX_SYS_NAME];
		// This is up to the reflection system of the scripting language, essentially, where in the array they are
		// supposed to go, which order it was added to the registry
		int32_t registryIndex;
	};

	class ScriptingHost
	{
	public:
		enum class EError
		{
			Ok = 0,
			SystemNotFound
		};
		// Lifetime
		using StartScriptingConnection_t = void (*)(HushFuncPtrTable *table, HushEngine *engine);
		using DisposeScriptingConnection_t = void (*)();

		// System data
		using GetAvailableSystemsFnPtr_t = bool (*)(ScriptingSystemInfo **outSystemInfoArr, uint64_t systemsCount);
		using GetSystemCountFnPtr_t = uint64_t (*)();
		using InstantiateSystemFnPtr_t = uint8_t *(*)(const ScriptingSystemInfo *systeminfo);

		static constexpr std::string_view FN_PTR_NOT_INITIALIZED_ERR =
			"Function pointer is not initialized! Forgot to call ScriptingHost::Initialize?";

		// We need to load an arbitrary DLL and communicate with it through C calls
		void Initialize(NullTerminatedStringView dllPath);

		std::vector<ScriptingSystemInfo> &GetAvailableSystems();

		Result<uintptr_t, EError> CreateSystem(const ScriptingSystemInfo &systemInfo);

		[[nodiscard]]
		StartScriptingConnection_t GetStartScriptingConnectionFn() const noexcept
		{
			HUSH_ASSERT(this->m_startScriptingConnectionFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return this->m_startScriptingConnectionFn;
		}

		[[nodiscard]]
		GetAvailableSystemsFnPtr_t GetAvailableSystemsFn() const noexcept
		{
			HUSH_ASSERT(m_getAvailableSystemsFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_getAvailableSystemsFn;
		}

		[[nodiscard]]
		GetSystemCountFnPtr_t GetSystemCountFn() const noexcept
		{
			HUSH_ASSERT(m_getSystemCountFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_getSystemCountFn;
		}

		[[nodiscard]]
		ScriptingSystemInterface *GetScriptingSystemInterface() noexcept
		{
			return &this->m_scriptingInterface;
		}

	private:
		void FetchSystemsIntoCache();

		bool m_libIsDirty = true;
		std::vector<ScriptingSystemInfo> m_availableSystems;

		StartScriptingConnection_t m_startScriptingConnectionFn = nullptr;
		DisposeScriptingConnection_t m_disposeScriptingConnectionFn = nullptr;

		GetAvailableSystemsFnPtr_t m_getAvailableSystemsFn = nullptr;
		GetSystemCountFnPtr_t m_getSystemCountFn = nullptr;
		InstantiateSystemFnPtr_t m_instantiateSystemFn = nullptr;

		ScriptingSystemInterface m_scriptingInterface;
	};
} // namespace Hush
