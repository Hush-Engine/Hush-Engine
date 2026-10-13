/*! \file Type.cpp
	\author Alan Ramirez
	\date 2025-04-15
	\brief Hush Engine Reflection Type
*/

#include "Type.hpp"
#include "crypto/Hashing.hpp"
#include "reflection/FieldInfo.hpp"
#include "reflection/TypeInfo.hpp"
#include "reflection/Variant.hpp"
#include <span>

Hush::Reflection::ERegisterClassError Hush::Reflection::ReflectionDB::RegisterClass(TypeInfo typeInfo,
																					ModuleHandle module)
{
	std::unique_lock lock(m_mutex);
	TypeId id = typeInfo.GetId();
	if (m_types.find(id) != m_types.end())
	{
		return ERegisterClassError::DuplicateType;
	}
	typeInfo.SetOwner(module);
	m_types.insert_or_assign(id, std::move(typeInfo));
	m_moduleTypes[module].push_back(id);
	return ERegisterClassError::None;
}

Hush::Reflection::ERegisterClassError Hush::Reflection::ReflectionDB::RegisterForeignClass(std::string_view name,
																						   size_t size,
																						   size_t alignment,
																						   ModuleHandle module)
{
	TypeInfo ti{{Hashing::Fnv1a64(name)}};
	ti.SetName(name);
	ti.SetSize(size);
	ti.SetAlignment(alignment);
	// BACKLOG: If the foreign module has constructors we can add them here, otherwise, 0 initialization is preferred
	return this->RegisterClass(ti, module);
}

bool Hush::Reflection::ReflectionDB::AddFieldToForeignClass(TypeId classType, std::string_view fieldName, TypeId type,
															uint64_t offset)
{
	TypeInfo *ti = this->GetTypeInfo(classType);
	if (ti == nullptr)
	{
		return false;
	}
	// HACK: Not doing anything for setters and getters yet
	FieldInfo fi{type, std::string(fieldName), [](std::span<const VariantView>) { return EVariantError::None; },
				 [](std::span<const VariantView>) -> Result<Variant, EVariantError> { return Variant{}; }, offset};
	ti->AddField(fi);
	return true;
}
