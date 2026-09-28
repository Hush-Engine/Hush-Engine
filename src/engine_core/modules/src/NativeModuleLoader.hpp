/*! \file NativeModuleLoader.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Loads native gameplay modules from dynamic libraries
*/

#pragma once

#include "HushModuleAbi.h"
#include "ModuleRegistry.hpp"

#include "Result.hpp"
#include "reflection/ModuleHandle.hpp"

#include <filesystem>
#include <string_view>

namespace Hush
{
	class HushEngine;
}

namespace Hush::Modules
{
	/// Loads native gameplay modules (C++, Rust cdylib, NativeAOT C#) from
	/// dynamic libraries. Every library must export the HushRegisterModule
	/// entry point.
	class NativeModuleLoader
	{
	public:
		enum class EError : std::uint8_t
		{
			None = 0,
			LibraryLoadFailed,
			EntryPointNotFound,
			AbiVersionMismatch,
			RegistrationFailed,
		};

		/// @param registry Registry the modules are registered into.
		/// @param engine Engine pointer stored in the module context. May be
		/// null in tests.
		/// @param hushApi Generated Hush API function table. May be null when
		/// the module does not use the generated API.
		NativeModuleLoader(ModuleRegistry &registry, HushEngine *engine, const HushFuncPtrTable *hushApi);

		/// Loads the library at the given path and calls its module entry
		/// point. When the entry point fails, everything the module
		/// registered is rolled back and the library is unloaded.
		///
		/// @param path Path of the dynamic library to load.
		/// @return The handle of the new module, or an error.
		Result<ModuleHandle, EError> Load(const std::filesystem::path &path, std::string_view moduleName = {},
										  EModuleKind kind = EModuleKind::NativeCpp);

	private:
		ModuleRegistry &m_registry;
		HushEngine *m_engine;
		const HushFuncPtrTable *m_hushApi;
	};
} // namespace Hush::Modules
