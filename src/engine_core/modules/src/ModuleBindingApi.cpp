/*! \file ModuleBindingApi.cpp
	\author Alan Ramirez
	\date 2026-08-23
	\brief C-binding entry points used by foreign gameplay modules
*/

#include "ModuleBindingApi.hpp"

#include "HushModuleAbi.h"
#include "HushEngine.hpp"
#include "ModuleRegistry.hpp"

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
		return value.size() > std::numeric_limits<std::uint32_t>::max()
				? 0
				: static_cast<std::uint32_t>(value.size());
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
	nativeDescriptor.name = std::string(abiDescriptor->name.data, abiDescriptor->name.size);
	nativeDescriptor.order = abiDescriptor->order;
	nativeDescriptor.lifecycleMask = abiDescriptor->lifecycleMask;

	return engine->GetModuleRegistry()->RegisterForeignSystem(module, std::move(nativeDescriptor)) ==
		   ModuleRegistry::EError::None;
}

bool Hush::Modules::AddSystemToScene(HushEngine *engine, Scene *scene, ModuleHandle module,
										 std::uint64_t typeId)
{
	if (engine == nullptr || scene == nullptr)
	{
		return false;
	}

	return engine->GetModuleRegistry()->AddSystemToScene(module, Reflection::TypeId{typeId}, *scene) ==
		   ModuleRegistry::EError::None;
}

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

bool Hush::Modules::HasReflectedTypeMetadata(HushEngine *engine, std::uint64_t typeId,
											 const char *keyData, std::size_t keySize)
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

bool Hush::Modules::CopyReflectedTypeMetadataValue(HushEngine *engine, std::uint64_t typeId,
														 const char *keyData, std::size_t keySize, char *destination,
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
