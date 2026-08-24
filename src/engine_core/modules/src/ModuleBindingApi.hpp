/*! \file ModuleBindingApi.hpp
	\author Alan Ramirez
	\date 2026-08-23
	\brief C-binding entry points used by foreign gameplay modules
*/

#pragma once

#include "HushBindings.hpp"
#include "reflection/ModuleHandle.hpp"

#include <cstddef>
#include <cstdint>

namespace Hush
{
	class HushEngine;
	class Scene;
} // namespace Hush

namespace Hush::Modules
{
	/// Stores the callback table of a foreign module. ops must point to a
	/// HushSystemRuntimeOps from HushModuleAbi.h.
	[[hush::export]]
	bool SetForeignSystemRuntimeOps(HushEngine *engine, ModuleHandle module, const void *ops);

	/// Registers a foreign system and its reflected type. descriptor must point
	/// to a HushSystemDescriptor from HushModuleAbi.h.
	[[hush::export]]
	bool RegisterForeignSystem(HushEngine *engine, ModuleHandle module, const void *descriptor);

	/// Creates a registered native or foreign system and adds it to the scene.
	/// The scene owns the resulting instance and manages its lifecycle.
	[[hush::export]]
	bool AddSystemToScene(HushEngine *engine, Scene *scene, ModuleHandle module, std::uint64_t typeId);

	/// Queries whether a reflected type is currently registered.
	[[hush::export]]
	bool HasReflectedType(HushEngine *engine, std::uint64_t typeId);

	/// Gets the module that owns a reflected type.
	[[hush::export]]
	bool GetReflectedTypeOwner(HushEngine *engine, std::uint64_t typeId, ModuleHandle *owner);

	/// Gets the byte length of a reflected type name, or zero when it is absent.
	[[hush::export]]
	std::uint32_t GetReflectedTypeNameLength(HushEngine *engine, std::uint64_t typeId);

	/// Copies a reflected type name into destination. destinationSize must be at
	/// least GetReflectedTypeNameLength for the same type.
	[[hush::export]]
	bool CopyReflectedTypeName(HushEngine *engine, std::uint64_t typeId, char *destination,
							   std::uint32_t destinationSize);

	/// Queries whether a reflected type has metadata with key.
	[[hush::export]]
	bool HasReflectedTypeMetadata(HushEngine *engine, std::uint64_t typeId, const char *keyData, std::size_t keySize);

	/// Gets the byte length of a type metadata value, or zero when it is absent.
	[[hush::export]]
	std::uint32_t GetReflectedTypeMetadataValueLength(HushEngine *engine, std::uint64_t typeId, const char *keyData,
													  std::size_t keySize);

	/// Copies a type metadata value into destination. destinationSize must be at
	/// least GetReflectedTypeMetadataValueLength for the same key.
	[[hush::export]]
	bool CopyReflectedTypeMetadataValue(HushEngine *engine, std::uint64_t typeId, const char *keyData,
										std::size_t keySize, char *destination, std::uint32_t destinationSize);
} // namespace Hush::Modules
