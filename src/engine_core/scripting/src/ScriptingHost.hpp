#pragma once

#include "Assertions.hpp"
#include "Result.hpp"
#include <string_view>
#include <vector>

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
		using StartScriptingConnection_t = void(*)(HushFuncPtrTable* table, HushEngine* engine);
		using DisposeScriptingConnection_t = void(*)();

		// System data
		using GetAvailableSystemsFnPtr_t = bool (*)(ScriptingSystemInfo **outSystemInfoArr, uint64_t systemsCount);
		using GetSystemCountFnPtr_t = uint64_t (*)();
		using InstantiateSystemFnPtr_t = uint8_t *(*)(const ScriptingSystemInfo *systeminfo);

		// System fn ptrs
		using CallSystemInit_t = void (*)(void *systemHandle);
		using CallSystemOnUpdate_t = void (*)(void *systemHandle, float delta);
		using CallSystemOnFixedUpdate_t = void (*)(void *systemHandle, float delta);
		using CallSystemOnShutdown_t = void (*)(void *systemHandle);
		using CallSystemOnRender_t = void (*)(void *systemHandle);
		using CallSystemOnPreRender_t = void (*)(void *systemHandle);
		using CallSystemOnPostRender_t = void (*)(void *systemHandle);

		static constexpr std::string_view FN_PTR_NOT_INITIALIZED_ERR = "Function pointer is not initialized! Forgot to call ScriptingHost::Initialize?";

		// We need to load an arbitrary DLL and communicate with it through C calls
		void Initialize(std::string_view dllPath);

		std::vector<ScriptingSystemInfo> &GetAvailableSystems();

		Result<uintptr_t, EError> CreateSystem(const ScriptingSystemInfo &systemInfo);

		[[nodiscard]]
		StartScriptingConnection_t GetStartScriptingConnectionFn() const noexcept {
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
		InstantiateSystemFnPtr_t GetInstantiateSystemFn() const noexcept
		{
			HUSH_ASSERT(m_instantiateSystemFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_instantiateSystemFn;
		}

		[[nodiscard]]
		CallSystemInit_t GetCallSystemInitFn() const noexcept
		{
			HUSH_ASSERT(m_callSystemInitFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_callSystemInitFn;
		}

		[[nodiscard]]
		CallSystemOnUpdate_t GetCallSystemOnUpdateFn() const noexcept
		{
			HUSH_ASSERT(m_callSystemOnUpdateFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_callSystemOnUpdateFn;
		}

		[[nodiscard]]
		CallSystemOnFixedUpdate_t GetCallSystemOnFixedUpdateFn() const noexcept
		{
			HUSH_ASSERT(m_callSystemOnFixedUpdateFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_callSystemOnFixedUpdateFn;
		}

		[[nodiscard]]
		CallSystemOnShutdown_t GetCallSystemOnShutdownFn() const noexcept
		{
			HUSH_ASSERT(m_callSystemOnShutdownFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_callSystemOnShutdownFn;
		}

		[[nodiscard]]
		CallSystemOnRender_t GetCallSystemOnRenderFn() const noexcept
		{
			HUSH_ASSERT(m_callSystemOnRenderFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_callSystemOnRenderFn;
		}

		[[nodiscard]]
		CallSystemOnPreRender_t GetCallSystemOnPreRenderFn() const noexcept
		{
			HUSH_ASSERT(m_callSystemOnPreRenderFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR.data());
			return m_callSystemOnPreRenderFn;
		}

		[[nodiscard]]
		CallSystemOnPostRender_t GetCallSystemOnPostRenderFn() const noexcept
		{
			HUSH_ASSERT(m_callSystemOnPostRenderFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR.data());
			return m_callSystemOnPostRenderFn;
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

		CallSystemInit_t m_callSystemInitFn = nullptr;
		CallSystemOnUpdate_t m_callSystemOnUpdateFn = nullptr;
		CallSystemOnFixedUpdate_t m_callSystemOnFixedUpdateFn = nullptr;
		CallSystemOnShutdown_t m_callSystemOnShutdownFn = nullptr;
		CallSystemOnRender_t m_callSystemOnRenderFn = nullptr;
		CallSystemOnPreRender_t m_callSystemOnPreRenderFn = nullptr;
		CallSystemOnPostRender_t m_callSystemOnPostRenderFn = nullptr;
	};
} // namespace Hush
