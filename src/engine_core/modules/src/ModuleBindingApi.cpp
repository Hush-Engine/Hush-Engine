/*! \file ModuleBindingApi.cpp
	\author Alan Ramirez
	\date 2026-08-23
	\brief C-binding entry points used by foreign gameplay modules
*/

#include "ModuleBindingApi.hpp"

#include "Assertions.hpp"
#include "Components/ComponentMetadata.hpp"
#include "Components/Serializable.hpp"
#include "HushModuleAbi.h"
#include "HushEngine.hpp"
#include "Logger.hpp"
#include "ModuleRegistry.hpp"
#include "reflection/FieldInfo.hpp"
#include "reflection/TypeId.hpp"
#include "reflection/TypeInfo.hpp"
#include "traits/EntityTraits.hpp"

#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <string_view>

namespace
{
	bool CopyString(std::string_view value, char *destination, std::uint32_t destinationSize)
	{
		if (destination == nullptr || destinationSize < value.size())
		{
			return false;
		}

		if (!value.empty())
		{
			std::memcpy(destination, value.data(), value.size());
		}
		return true;
	}

	std::uint32_t GetStringLength(std::string_view value)
	{
		return value.size() > std::numeric_limits<std::uint32_t>::max() ? 0 : static_cast<std::uint32_t>(value.size());
	}

	Hush::Reflection::ReflectionDB *GetReflectionDB(Hush::HushEngine *engine)
	{
		return engine == nullptr ? nullptr : engine->GetReflectionDB();
	}
} // namespace

bool Hush::Modules::SetForeignSystemRuntimeOps(HushEngine *engine, ModuleHandle module, const void *ops)
{
	if (engine == nullptr || ops == nullptr)
	{
		return false;
	}

	const auto *runtimeOps = static_cast<const HushSystemRuntimeOps *>(ops);
	if (runtimeOps->create == nullptr || runtimeOps->destroy == nullptr)
	{
		return false;
	}

	return engine->GetModuleRegistry()->SetSystemRuntimeOps(module, *runtimeOps) == ModuleRegistry::EError::None;
}

bool Hush::Modules::RegisterForeignSystem(HushEngine *engine, ModuleHandle module, const void *descriptor)
{
	if (engine == nullptr || descriptor == nullptr)
	{
		return false;
	}

	const auto *abiDescriptor = static_cast<const HushSystemDescriptor *>(descriptor);
	constexpr std::uint32_t VALID_LIFECYCLE_MASK = (HushSystemLifecycle_PostRender << 1) - 1;
	if (abiDescriptor->structSize < sizeof(HushSystemDescriptor) ||
		abiDescriptor->abiVersion != HUSH_MODULE_ABI_VERSION || abiDescriptor->name.data == nullptr ||
		abiDescriptor->name.size == 0 || abiDescriptor->order > ISystem::MAX_ORDER ||
		(abiDescriptor->lifecycleMask & ~VALID_LIFECYCLE_MASK) != 0)
	{
		return false;
	}

	ForeignSystemDescriptor nativeDescriptor;
	nativeDescriptor.typeId = Reflection::TypeId{abiDescriptor->typeId};
	nativeDescriptor.name = std::string_view(abiDescriptor->name.data, abiDescriptor->name.size);
	nativeDescriptor.order = abiDescriptor->order;
	nativeDescriptor.lifecycleMask = abiDescriptor->lifecycleMask;

	return engine->GetModuleRegistry()->RegisterForeignSystem(module, std::move(nativeDescriptor)) ==
		   ModuleRegistry::EError::None;
}

bool Hush::Modules::AddSystemToScene(HushEngine *engine, Scene *scene, ModuleHandle module, std::uint64_t typeId)
{
	if (engine == nullptr || scene == nullptr)
	{
		return false;
	}

	return engine->GetModuleRegistry()->AddSystemToScene(module, Reflection::TypeId{typeId}, *scene) ==
		   ModuleRegistry::EError::None;
}

// HACK: This macro ignores the error of the serializer
#define WRITE_PROP_OF_TYPE(ptr, name, type, serializer)                                                                \
	{                                                                                                                  \
		type value{};                                                                                                  \
		value = *(reinterpret_cast<const type *>(ptr));                                                                \
		(void)serializer.Serialize(name, value);                                                                       \
	}

inline void ScriptSidePropertySerialize(const uint8_t *instance, const Hush::Reflection::FieldInfo &propInfo,
										Hush::Serialization::JsonSerializer &serializer)
{
	using namespace Hush;

	uint64_t offset = propInfo.GetOffset();

	const uint8_t *ptrToRead = instance + offset;

	switch (propInfo.GetTypeId().id)
	{
	case Hush::EComponentPropertyType::Unknown:
		LogError("Unsupported type from the compile time scripting side reflection!");
		break;
	case Hush::EComponentPropertyType::I8:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), int8_t, serializer);
		break;
	case Hush::EComponentPropertyType::U8:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), uint8_t, serializer);
		break;
	case Hush::EComponentPropertyType::I16:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), int16_t, serializer);
		break;
	case Hush::EComponentPropertyType::U16:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), uint16_t, serializer);
		break;
	case Hush::EComponentPropertyType::I32:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), int32_t, serializer);
		break;
	case Hush::EComponentPropertyType::U32:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), uint32_t, serializer);
		break;
	case Hush::EComponentPropertyType::I64:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), int64_t, serializer);
		break;
	case Hush::EComponentPropertyType::U64:
		// Idk what to do abt u, I guess a string(?
		break;
	case Hush::EComponentPropertyType::F32:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), float, serializer);
		break;
	case Hush::EComponentPropertyType::F64:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), double, serializer);
		break;
	case Hush::EComponentPropertyType::Bool:
		WRITE_PROP_OF_TYPE(ptrToRead, propInfo.GetName(), bool, serializer);
		break;
	case Hush::EComponentPropertyType::String:
	case Hush::EComponentPropertyType::Array:
		// NYI: Array types
		break;
	}
}

void ScriptSidePropertyDeserialize(uint8_t *instance, const Hush::Reflection::TypeInfo *compSerialInfo,
								   Hush::Serialization::JsonDeserializer &deserializer)
{
	using namespace Hush;

	// Consume the key (the current token must be EToken::Key)
	std::string_view key = deserializer.GetKey();
	if (key.empty())
	{
		// Error: unexpected token
		return;
	}

	// Search for a matching property
	std::span<const Reflection::FieldInfo> fields = compSerialInfo->GetFields();
	for (const Reflection::FieldInfo &field : fields)
	{
		std::string_view propName = field.GetName(); // assuming field.GetName() is a null‑terminated char array

		if (propName == key)
		{
			// Found – write the value
			uint8_t *ptrToWrite = instance + field.GetOffset();

			// The current token after ReadKey() is the value.
			// Use the appropriate Read... method based on prop->type.
			switch (field.GetTypeId().id)
			{
			case Hush::EComponentPropertyType::I8: {
				int64_t temp{};
				deserializer.ReadInt(temp);
				*reinterpret_cast<int8_t *>(ptrToWrite) = static_cast<int8_t>(temp);
				break;
			}
			case Hush::EComponentPropertyType::U8: {
				int64_t temp{};
				deserializer.ReadInt(temp);
				*reinterpret_cast<uint8_t *>(ptrToWrite) = static_cast<uint8_t>(temp);
				break;
			}
			case Hush::EComponentPropertyType::I16: {
				int64_t temp{};
				deserializer.ReadInt(temp);
				*reinterpret_cast<int16_t *>(ptrToWrite) = static_cast<int16_t>(temp);
				break;
			}
			case Hush::EComponentPropertyType::U16: {
				int64_t temp{};
				deserializer.ReadInt(temp);
				*reinterpret_cast<uint16_t *>(ptrToWrite) = static_cast<uint16_t>(temp);
				break;
			}
			case Hush::EComponentPropertyType::I32: {
				int64_t temp{};
				deserializer.ReadInt(temp);
				*reinterpret_cast<int32_t *>(ptrToWrite) = static_cast<int32_t>(temp);
				break;
			}
			case Hush::EComponentPropertyType::U32: {
				*reinterpret_cast<uint32_t *>(ptrToWrite) = static_cast<uint32_t>(deserializer.GetUint());
				break;
			}
			case Hush::EComponentPropertyType::I64: {
				int64_t temp{};
				deserializer.ReadInt(temp);
				*reinterpret_cast<int64_t *>(ptrToWrite) = temp;
				break;
			}
			case Hush::EComponentPropertyType::U64:
				// NYI
				break;
			case Hush::EComponentPropertyType::F32: {
				double temp{};
				deserializer.ReadDouble(temp);
				*reinterpret_cast<float *>(ptrToWrite) = static_cast<float>(temp);
				break;
			}
			case Hush::EComponentPropertyType::F64: {
				double temp{};
				deserializer.ReadDouble(temp);
				*reinterpret_cast<double *>(ptrToWrite) = temp;
				break;
			}
			case Hush::EComponentPropertyType::Bool: {
				bool temp{};
				deserializer.ReadBool(temp);
				*reinterpret_cast<bool *>(ptrToWrite) = temp;
				break;
			}
			case Hush::EComponentPropertyType::String:
			case Hush::EComponentPropertyType::Array:
				// NYI
				break;
			default:
				LogError("Unsupported type from compile‑time reflection!");
				break;
			}
			return; // property handled
		}
	}

	// Property not found – skip the entire value (scalar, object, or array)
	deserializer.SkipValue();
}

bool Hush::Modules::RegisterComponentFromReflectedType(HushEngine *engine, Scene *scene, uint64_t typeId)
{
	if (engine == nullptr || scene == nullptr)
	{
		return false;
	}

	Reflection::TypeId reflectedTypeId{typeId};
	Reflection::TypeInfo *ti = engine->GetReflectionDB()->GetTypeInfo(reflectedTypeId);

	if (ti == nullptr)
	{
		return false;
	}

	ComponentTraits::ComponentInfo desc{};
	desc.alignment = ti->GetAlignment();
	desc.size = ti->GetSize();
	desc.name = ti->GetName().c_str();
	desc.ops.ctor = [](void *ptr, int count, const void *rawDesc) {
		const auto *localDesc = reinterpret_cast<const ComponentTraits::ComponentInfo *>(rawDesc);
		// A simple memset for now to 0 init is fine
		std::memset(ptr, 0, count * localDesc->size);
	};
	Entity compEnt = scene->EntityFromIdUnchecked(scene->RegisterComponentRaw(desc));
	// TODO: Now add Inspectable and Serializable comps
	InspectableComponent &inspectableComp = compEnt.AddComponent<InspectableComponent>();
	inspectableComp.reflectedTypeId = reflectedTypeId;

	Serializable &ser = compEnt.AddComponent<Serializable>();

	struct LocalSerializationCtx
	{
		Reflection::TypeId reflectedType;
		HushEngine *engine = nullptr;
	};
	static_assert(sizeof(LocalSerializationCtx) <= Serializable::LOCAL_BUFF_SIZE,
				  "Local Serialization context is too large for the local buffer of the Serializable component!");

	LocalSerializationCtx serCtx{};
	serCtx.reflectedType = reflectedTypeId;
	serCtx.engine = engine;

	ser.SetContextInline(reinterpret_cast<const uint8_t *>(&serCtx), sizeof(LocalSerializationCtx));

	ser.serialize = [](const uint8_t *self, Serialization::JsonSerializer &serializer, void *ctx) {
		auto *localCtx = reinterpret_cast<LocalSerializationCtx *>(ctx);
		if (localCtx->engine == nullptr)
		{
			return Serializable::EError::MissingInternalResource;
		}

		const Reflection::TypeInfo *localTi = localCtx->engine->GetReflectionDB()->GetTypeInfo(localCtx->reflectedType);
		HUSH_ASSERT(localTi != nullptr,
					"A serializable type ({}) is lost, this shouldn't happen, all registered components must remain "
					"registered throughout the lifetime of the module!",
					localCtx->reflectedType.id);

		std::span<const Reflection::FieldInfo> fields = localTi->GetFields();

		for (const Reflection::FieldInfo &field : fields)
		{
			ScriptSidePropertySerialize(self, field, serializer);
		}

		return Serializable::EError::None;
	};

	// TODO: Deserialize
	ser.deserialize = [](uint8_t *self, Serialization::JsonDeserializer &deserializer, void *ctx) {
		auto *localCtx = reinterpret_cast<LocalSerializationCtx *>(ctx);

		if (localCtx->engine == nullptr)
		{
			return Serializable::EError::MissingInternalResource;
		}

		const Reflection::TypeInfo *localTi = localCtx->engine->GetReflectionDB()->GetTypeInfo(localCtx->reflectedType);
		HUSH_ASSERT(localTi != nullptr,
					"A serializable type ({}) is lost, this shouldn't happen, all registered components must remain "
					"registered throughout the lifetime of the module!",
					localCtx->reflectedType.id);

		deserializer.Next();

		while (deserializer.Next())
		{
			auto token = deserializer.GetToken();

			if (token == Serialization::JsonDeserializer::EToken::ObjectEnd)
			{
				break; // done with this object
			}

			if (token == Serialization::JsonDeserializer::EToken::Key)
			{
				// The function will read the key and then the value (or skip it)
				ScriptSidePropertyDeserialize(self, localTi, deserializer);
			}
			else
			{
				// Unexpected token (shouldn't happen in valid JSON) – skip it
				deserializer.SkipValue();
			}
		}

		return Serializable::EError::None;
	};

	return true;
}

#undef WRITE_PROP_OF_TYPE

bool Hush::Modules::HasReflectedType(HushEngine *engine, std::uint64_t typeId)
{
	Reflection::ReflectionDB *db = GetReflectionDB(engine);
	return db != nullptr && db->HasType(Reflection::TypeId{typeId});
}

bool Hush::Modules::GetReflectedTypeOwner(HushEngine *engine, std::uint64_t typeId, ModuleHandle *owner)
{
	Reflection::ReflectionDB *db = GetReflectionDB(engine);
	if (db == nullptr || owner == nullptr)
	{
		return false;
	}

	const std::optional<ModuleHandle> value = db->GetTypeOwner(Reflection::TypeId{typeId});
	if (!value.has_value())
	{
		return false;
	}
	*owner = *value;
	return true;
}

std::uint32_t Hush::Modules::GetReflectedTypeNameLength(HushEngine *engine, std::uint64_t typeId)
{
	Reflection::ReflectionDB *db = GetReflectionDB(engine);
	if (db == nullptr)
	{
		return 0;
	}
	const std::optional<std::string> name = db->GetTypeName(Reflection::TypeId{typeId});
	return name.has_value() ? GetStringLength(*name) : 0;
}

bool Hush::Modules::CopyReflectedTypeName(HushEngine *engine, std::uint64_t typeId, char *destination,
										  std::uint32_t destinationSize)
{
	Reflection::ReflectionDB *db = GetReflectionDB(engine);
	if (db == nullptr)
	{
		return false;
	}
	const std::optional<std::string> name = db->GetTypeName(Reflection::TypeId{typeId});
	return name.has_value() && CopyString(*name, destination, destinationSize);
}

bool Hush::Modules::HasReflectedTypeMetadata(HushEngine *engine, std::uint64_t typeId, const char *keyData,
											 std::size_t keySize)
{
	Reflection::ReflectionDB *db = GetReflectionDB(engine);
	return db != nullptr && keyData != nullptr &&
		   db->HasTypeMetadata(Reflection::TypeId{typeId}, std::string_view(keyData, keySize));
}

std::uint32_t Hush::Modules::GetReflectedTypeMetadataValueLength(HushEngine *engine, std::uint64_t typeId,
																 const char *keyData, std::size_t keySize)
{
	Reflection::ReflectionDB *db = GetReflectionDB(engine);
	if (db == nullptr || keyData == nullptr)
	{
		return 0;
	}

	const std::optional<std::string> value =
		db->GetTypeMetadata(Reflection::TypeId{typeId}, std::string_view(keyData, keySize));
	return value.has_value() ? GetStringLength(*value) : 0;
}

bool Hush::Modules::CopyReflectedTypeMetadataValue(HushEngine *engine, std::uint64_t typeId, const char *keyData,
												   std::size_t keySize, char *destination,
												   std::uint32_t destinationSize)
{
	Reflection::ReflectionDB *db = GetReflectionDB(engine);
	if (db == nullptr || keyData == nullptr)
	{
		return false;
	}

	const std::optional<std::string> value =
		db->GetTypeMetadata(Reflection::TypeId{typeId}, std::string_view(keyData, keySize));
	return value.has_value() && CopyString(*value, destination, destinationSize);
}
