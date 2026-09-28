/*! \file NativeModuleEntry.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Helper used by the generated entry point of native C++ modules
*/

#pragma once

#include "HushModuleAbi.h"
#include "ModuleRegistry.hpp"

#include <span>

namespace Hush
{
	class HushEngine;
}

namespace Hush::Modules
{
	/// Engine side objects a native module needs during registration. A
	/// pointer to this struct is stored in HushModuleContext::hostData while
	/// the entry point runs. Only native C++ modules may use it, foreign
	/// language modules must go through the generated Hush API.
	///
	/// Native C++ modules are always built against the exact same engine
	/// version as the host that loads them.
	struct NativeModuleHost
	{
		HushEngine *engine;
		Reflection::ReflectionDB *reflectionDB;
		ModuleRegistry *moduleRegistry;
	};

	/// Runs the registration of a native C++ module. The reflection generator
	/// emits a HushRegisterModule function that forwards to this helper.
	///
	/// @param context Context received by the module entry point.
	/// @param registerTypes Generated function that registers the module types.
	/// @param systems Generated descriptors of the module systems.
	inline HushModuleResult NativeModuleEntry(const HushModuleContext *context,
											  bool (*registerTypes)(Reflection::ReflectionDB &, ModuleHandle),
											  std::span<const SystemDescriptor> systems)
	{
		if (context == nullptr || context->abiVersion != HUSH_MODULE_ABI_VERSION)
		{
			return HushModuleResult_AbiVersionMismatch;
		}

		auto *host = static_cast<NativeModuleHost *>(context->hostData);
		if (host == nullptr || host->reflectionDB == nullptr || host->moduleRegistry == nullptr)
		{
			return HushModuleResult_RegistrationFailed;
		}

		if (!registerTypes(*host->reflectionDB, context->module))
		{
			return HushModuleResult_RegistrationFailed;
		}

		if (host->moduleRegistry->RegisterNativeSystems(context->module, systems) != ModuleRegistry::EError::None)
		{
			return HushModuleResult_RegistrationFailed;
		}

		return HushModuleResult_Ok;
	}
} // namespace Hush::Modules
