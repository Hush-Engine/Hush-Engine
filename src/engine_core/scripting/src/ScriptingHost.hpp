#pragma once

#include "Assertions.hpp"
#include "NullTerminatedStringView.hpp"
#include "Result.hpp"
#include "VirtualFilesystem.hpp"
#include <cstdint>
#include <string_view>
#include <vector>
#include <ISystem.hpp>

// fwd declaration
struct HushFuncPtrTable;

namespace Hush
{

	class HushEngine;

	/// @brief Used for both the system and the component registry that is generated at compile time
	struct ScriptingRegisteredTypeInfo
	{
		static constexpr int32_t MAX_TYPE_NAME = 64;
		char name[MAX_TYPE_NAME];
		// This is up to the reflection system of the scripting language, essentially, where in the array they are
		// supposed to go, which order it was added to the registry
		int32_t registryIndex;
		uint64_t byteSize;
		uint64_t align;
	};

	/// @brief Mirrors ECompPropertyType from the scripting library's compile-time serialization registry
	enum class EComponentPropertyType : int32_t
	{
		Unknown = 0,
		I8,
		U8,
		I16,
		U16,
		I32,
		U32,
		I64,
		U64,
		F32,
		F64,
		Bool,
		String,
		Array,
	};

	/// @brief Mirrors CompPropertyInfo, describes a single serializable field of a scripted component
	struct ScriptingComponentPropertyInfo
	{
		static constexpr int32_t MAX_PROP_NAME = 64;
		char name[MAX_PROP_NAME];
		EComponentPropertyType type;
		uint32_t elementCount;
		uint32_t elementSize;
		uint32_t offset;
	};

	/// @brief Mirrors CompSerializationInfo, compile-time reflection data for serializing scripted components
	struct ScriptingComponentSerializationInfo
	{
		static constexpr int32_t MAX_COMP_NAME = 64;
		char name[MAX_COMP_NAME];
		int32_t registryIndex;
		uint64_t byteSize;
		uint64_t align;
		ScriptingComponentPropertyInfo *properties;
		uint32_t propertyCount;
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
		using GetAvailableRegisterTypesFnPtr_t = void (*)(ScriptingRegisteredTypeInfo **outRegisterTypeInfoArr,
														  uint64_t systemsCount);
		using GetRegisterTypeCountFnPtr_t = uint64_t (*)();
		using InstantiateSystemFnPtr_t = uint8_t *(*)(const ScriptingRegisteredTypeInfo *systeminfo);

		// Component serialization data
		using GetAvailableComponentSerializationDataFnPtr_t = void (*)(ScriptingComponentSerializationInfo **outInfoArr,
																	   uint64_t capacity);
		using GetComponentSerializationCountFnPtr_t = uint64_t (*)();

		static constexpr std::string_view FN_PTR_NOT_INITIALIZED_ERR =
			"Function pointer is not initialized! Forgot to call ScriptingHost::Initialize?";

		// We need to load an arbitrary DLL and communicate with it through C calls
		void Initialize(NullTerminatedStringView dllPath, VirtualFilesystem *vfs, Scene *scene);

		std::vector<ScriptingRegisteredTypeInfo> &GetAvailableSystems();

		std::vector<ScriptingRegisteredTypeInfo> &GetAvailableComponents();

		std::vector<ScriptingComponentSerializationInfo> &GetAvailableComponentSerializationData();

		Result<uintptr_t, EError> CreateSystem(const ScriptingRegisteredTypeInfo &systemInfo);

		[[nodiscard]]
		StartScriptingConnection_t GetStartScriptingConnectionFn() const noexcept
		{
			HUSH_ASSERT(this->m_startScriptingConnectionFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return this->m_startScriptingConnectionFn;
		}

		[[nodiscard]]
		GetAvailableRegisterTypesFnPtr_t GetAvailableSystemsFn() const noexcept
		{
			HUSH_ASSERT(m_getAvailableSystemsFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_getAvailableSystemsFn;
		}

		[[nodiscard]]
		GetRegisterTypeCountFnPtr_t GetSystemCountFn() const noexcept
		{
			HUSH_ASSERT(m_getSystemCountFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_getSystemCountFn;
		}

		[[nodiscard]]
		GetRegisterTypeCountFnPtr_t GetComponentCountFn() const noexcept
		{
			HUSH_ASSERT(m_getComponentCountFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_getComponentCountFn;
		}

		[[nodiscard]]
		GetComponentSerializationCountFnPtr_t GetComponentSerializationCountFn() const noexcept
		{
			HUSH_ASSERT(m_getComponentSerializationCountFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_getComponentSerializationCountFn;
		}

		[[nodiscard]]
		GetAvailableComponentSerializationDataFnPtr_t GetAvailableComponentSerializationDataFn() const noexcept
		{
			HUSH_ASSERT(m_getAvailableComponentSerializationDataFn != nullptr, "{}", FN_PTR_NOT_INITIALIZED_ERR);
			return m_getAvailableComponentSerializationDataFn;
		}

		[[nodiscard]]
		ScriptingSystemInterface *GetScriptingSystemInterface() noexcept
		{
			return &this->m_scriptingInterface;
		}

	private:
		void FetchSystemsIntoCache();

		void FetchComponentsIntoCache();

		void FetchComponentSerializationIntoCache();

		bool m_libIsDirty = true;
		std::vector<ScriptingRegisteredTypeInfo> m_availableSystems;
		std::vector<ScriptingRegisteredTypeInfo> m_availableComponents;
		std::vector<ScriptingComponentSerializationInfo> m_componentSerializationInfos;

		StartScriptingConnection_t m_startScriptingConnectionFn = nullptr;
		DisposeScriptingConnection_t m_disposeScriptingConnectionFn = nullptr;

		GetAvailableRegisterTypesFnPtr_t m_getAvailableSystemsFn = nullptr;
		GetRegisterTypeCountFnPtr_t m_getSystemCountFn = nullptr;
		InstantiateSystemFnPtr_t m_instantiateSystemFn = nullptr;

		GetAvailableRegisterTypesFnPtr_t m_getAvailableComponentsFn = nullptr;
		GetRegisterTypeCountFnPtr_t m_getComponentCountFn = nullptr;

		GetComponentSerializationCountFnPtr_t m_getComponentSerializationCountFn = nullptr;
		GetAvailableComponentSerializationDataFnPtr_t m_getAvailableComponentSerializationDataFn = nullptr;

		ScriptingSystemInterface m_scriptingInterface;
	};
} // namespace Hush
