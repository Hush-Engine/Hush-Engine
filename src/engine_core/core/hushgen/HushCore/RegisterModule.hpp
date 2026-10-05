// AUTO-GENERATED -- DO NOT EDIT
#pragma once

#include <reflection/ModuleHandle.hpp>

#include <span>

namespace Hush
{
	struct SystemDescriptor;
}

namespace Hush::Reflection
{
	class ReflectionDB;
}

/// Registers every reflected type of the HushCore module into the reflection database.
/// The module handle says which module owns the types.
/// Returns true when every type was registered.
bool RegisterModule_HushCore(Hush::Reflection::ReflectionDB &db, Hush::ModuleHandle module = Hush::ENGINE_MODULE_HANDLE);

/// Gets the descriptors of every system defined by the HushCore module.
std::span<const Hush::SystemDescriptor> GetSystemDescriptors_HushCore();
