#include "ScriptingHost.hpp"
#include "Assertions.hpp"
#include "Components/ComponentMetadata.hpp"
#include "Components/Serializable.hpp"
#include "LibManager.hpp"
#include "Logger.hpp"
#include "Scene.hpp"
#include "VirtualFilesystem.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include <cstdint>
#include <cstring>

constexpr std::string_view START_SCRIPTING_CONNECTION = "StartScriptingConnection";
constexpr std::string_view DISPOSE_SCRIPTING_CONNECTION = "DisposeScriptingConnection";

constexpr std::string_view GET_AVAILABLE_SYSTEMS_FN_NAME = "GetAvailableSystems";
constexpr std::string_view GET_SYSTEM_COUNT_FN_NAME = "GetSystemCount";

constexpr std::string_view GET_AVAILABLE_COMPONENTS_FN_NAME = "GetAvailableComponents";
constexpr std::string_view GET_COMPONENT_COUNT_FN_NAME = "GetComponentCount";

constexpr std::string_view GET_AVAILABLE_COMPONENT_SERIALIZATION_DATA_FN_NAME =
	"GetAvailableComponentSerializationData";
constexpr std::string_view GET_COMPONENT_SERIALIZATION_COUNT_FN_NAME = "GetComponentSerializationCount";

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
inline size_t ScriptPropertyTypeGetSize(Hush::EComponentPropertyType type) {
	using namespace Hush;

	switch (type) {
	case Hush::EComponentPropertyType::Unknown:
		LogError("Unsupported type from the compile time scripting side reflection!");
		return size_t(-1);
	case Hush::EComponentPropertyType::I8:
		return 8;
	case Hush::EComponentPropertyType::U8:
		return 8;
	case Hush::EComponentPropertyType::I16:
		return 16;
	case Hush::EComponentPropertyType::U16:
		return 16;
	case Hush::EComponentPropertyType::I32:
		return 32;
	case Hush::EComponentPropertyType::U32:
		return 32;
	case Hush::EComponentPropertyType::I64:
		return 64;
	case Hush::EComponentPropertyType::U64:
		return 64;
	case Hush::EComponentPropertyType::F32:
		return 32;
	case Hush::EComponentPropertyType::F64:
		return 64;
	case Hush::EComponentPropertyType::Bool:
		return 8;
	case Hush::EComponentPropertyType::String:
	case Hush::EComponentPropertyType::Array:
		// NYI: Array types
		return size_t(-1);
	}
}

// HACK: This macro ignores the error of the serializer
#define WRITE_PROP_OF_TYPE(ptr, name, type, serializer) {\
	type value {};\
	value = *(reinterpret_cast<const type *>(ptr));\
	(void)serializer.Serialize(name, value);\
}

inline void ScriptSidePropertySerialize(const uint8_t* instance, const Hush::ScriptingComponentPropertyInfo* propInfo, Hush::Serialization::JsonSerializer& serializer) {
	using namespace Hush;

	uint32_t offset = propInfo->offset;

	const uint8_t* ptrToRead = instance + offset;

	switch (propInfo->type) {
	case Hush::EComponentPropertyType::Unknown:
		LogError("Unsupported type from the compile time scripting side reflection!");
		break;
	case Hush::EComponentPropertyType::I8:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, int8_t, serializer);
		break;
	case Hush::EComponentPropertyType::U8:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, uint8_t, serializer);
		break;
	case Hush::EComponentPropertyType::I16:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, int16_t, serializer);
		break;
	case Hush::EComponentPropertyType::U16:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, uint16_t, serializer);
		break;
	case Hush::EComponentPropertyType::I32:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, int32_t, serializer);
		break;
	case Hush::EComponentPropertyType::U32:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, uint32_t, serializer);
		break;
	case Hush::EComponentPropertyType::I64:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, int64_t, serializer);
		break;
	case Hush::EComponentPropertyType::U64:
		// Idk what to do abt u, I guess a string(?
		break;
	case Hush::EComponentPropertyType::F32:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, float, serializer);
		break;
	case Hush::EComponentPropertyType::F64:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, double, serializer);
		break;
	case Hush::EComponentPropertyType::Bool:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo->name, bool, serializer);
		break;
	case Hush::EComponentPropertyType::String:
	case Hush::EComponentPropertyType::Array:
		// NYI: Array types
		break;
	}
}

#undef WRITE_PROP_OF_TYPE

void Hush::ScriptingHost::Initialize(NullTerminatedStringView dllPath, VirtualFilesystem *vfs, Scene *scene)
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

	BIND_DLL_SCRIPTING_FUNCTION(GET_AVAILABLE_COMPONENT_SERIALIZATION_DATA_FN_NAME,
								this->m_getAvailableComponentSerializationDataFn);
	BIND_DLL_SCRIPTING_FUNCTION(GET_COMPONENT_SERIALIZATION_COUNT_FN_NAME, this->m_getComponentSerializationCountFn);

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

	// This can be discarded
	std::vector<Hush::ScriptingRegisteredTypeInfo> &comps = this->GetAvailableComponents();
	// This one should stay
	std::vector<ScriptingComponentSerializationInfo>& compSerialInfo = this->GetAvailableComponentSerializationData();

	HUSH_ASSERT(comps.size() == compSerialInfo.size(), "Component registration info and serialization info should be parallel arrays (same size)");

	for (size_t i = 0; i < compSerialInfo.size(); i++)
	{
		ScriptingComponentSerializationInfo& serializationInfo = compSerialInfo[i];

		auto nameView = std::string_view(static_cast<const char *>(serializationInfo.name));

		// Register the component
		Entity::EntityId compId = scene->RegisterComponentRaw({
			.size = serializationInfo.byteSize,
			.alignment = serializationInfo.align,
			.name = nameView.data(),
		});

		Entity comp = scene->EntityFromIdUnchecked(compId);

		// Add the inspectable tag
		comp.AddComponent<InspectableComponent>();
		// Add serialization info from the comptime serialization
		Serializable& ser = comp.AddComponent<Serializable>();
		// The current model stores this info on static memory on the scripting side, so this pointer should be valid throughout the lifetime of the host
		ser.ctx = &serializationInfo;
		ser.serialize = [](const uint8_t* self, Serialization::JsonSerializer& serializer, void* ctx) {
			auto* serializationInfo = reinterpret_cast<ScriptingComponentSerializationInfo*>(ctx);

			uint32_t propCount = serializationInfo->propertyCount;

			for (uint32_t i = 0; i < propCount; i++) {
				ScriptingComponentPropertyInfo* propInfo = &(serializationInfo->properties[i]);
				ScriptSidePropertySerialize(self, propInfo, serializer);
			}

			return Serializable::EError::None;
		};
		// TODO: Deserialize
	
	}
}
// NOLINTEND

std::vector<Hush::ScriptingRegisteredTypeInfo> &Hush::ScriptingHost::GetAvailableSystems()
{
	if (m_libIsDirty)
	{
		this->FetchSystemsIntoCache();
		this->FetchComponentsIntoCache();
		this->FetchComponentSerializationIntoCache();
	}
	return this->m_availableSystems;
}

std::vector<Hush::ScriptingRegisteredTypeInfo> &Hush::ScriptingHost::GetAvailableComponents()
{
	if (m_libIsDirty)
	{
		this->FetchComponentsIntoCache();
		this->FetchSystemsIntoCache();
		this->FetchComponentSerializationIntoCache();
	}
	return this->m_availableComponents;
}

std::vector<Hush::ScriptingComponentSerializationInfo> &Hush::ScriptingHost::GetAvailableComponentSerializationData()
{
	if (m_libIsDirty)
	{
		this->FetchComponentSerializationIntoCache();
		this->FetchComponentsIntoCache();
		this->FetchSystemsIntoCache();
	}
	return this->m_componentSerializationInfos;
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
	HUSH_ASSERT(
		this->m_getComponentCountFn != nullptr,
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

void Hush::ScriptingHost::FetchComponentSerializationIntoCache()
{
	HUSH_ASSERT(this->m_getComponentSerializationCountFn != nullptr,
				"Function pointer to get component serialization count is not initialized, forgot to call "
				"ScriptingHost::Initialize?");
	uint64_t serializationCount = this->m_getComponentSerializationCountFn();
	Hush::LogFormat(ELogLevel::Info, "Got {} component serialization infos from the scripting system!",
					serializationCount);
	this->m_componentSerializationInfos.resize(serializationCount);

	HUSH_ASSERT(this->m_getAvailableComponentSerializationDataFn != nullptr,
				"Function pointer to get available component serialization data is not initialized, forgot to call "
				"ScriptingHost::Initialize?");
	ScriptingComponentSerializationInfo *serializationArr = this->m_componentSerializationInfos.data();

	this->m_getAvailableComponentSerializationDataFn(&serializationArr, this->m_componentSerializationInfos.size());
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
