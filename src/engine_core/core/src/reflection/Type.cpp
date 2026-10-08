/*! \file Type.cpp
	\author Alan Ramirez
	\date 2025-04-15
	\brief Hush Engine Reflection Type
*/

#include "Type.hpp"
#include "crypto/Hashing.hpp"
#include "reflection/TypeInfo.hpp"

Hush::Reflection::ERegisterClassError Hush::Reflection::ReflectionDB::RegisterClass(TypeInfo typeInfo, ModuleHandle module) 
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


Hush::Reflection::ERegisterClassError Hush::Reflection::ReflectionDB::RegisterForeignClass(std::string_view name, size_t size, size_t alignment, ModuleHandle module) {
	TypeInfo ti{ { Hashing::Fnv1a64(name) }  };
	ti.SetName(name);
	ti.SetSize(size);
	ti.SetAlignment(alignment);
	// BACKLOG: If the foreign module has constructors we can add them here, otherwise, 0 initialization is preferred
	return this->RegisterClass(ti, module);
}
