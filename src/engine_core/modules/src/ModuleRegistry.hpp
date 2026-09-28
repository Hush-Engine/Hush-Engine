/*! \file ModuleRegistry.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Registry of the gameplay modules loaded by the engine
*/

#pragma once

#include "HushModuleAbi.h"

#include "ISystem.hpp"
#include "Result.hpp"
#include "SystemDescriptor.hpp"
#include "reflection/ModuleHandle.hpp"
#include "reflection/Type.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Hush
{
	class Scene;
	class HushEngine;
	class SharedLibrary;
} // namespace Hush

namespace Hush::Modules
{
	/// Kind of runtime a module is implemented with. This is only loader
	/// metadata, the engine never schedules systems differently based on it.
	enum class EModuleKind : std::uint8_t
	{
		NativeCpp,
		Rust,
		DotNetCoreClr,
		DotNetNativeAot,
	};

	/// Registration state of a module record.
	enum class EModuleState : std::uint8_t
	{
		/// The module is registering its types right now. A module in this
		/// state is not visible to the rest of the engine.
		Registering,
		/// The module finished registering and can be used.
		Active,
	};

	/// Description of a system implemented in a foreign language.
	struct ForeignSystemDescriptor
	{
		Reflection::TypeId typeId;
		std::string name;
		std::uint16_t order = 0;
		std::uint32_t lifecycleMask = 0;
	};

	/// Everything the engine knows about one loaded module.
	struct ModuleRecord
	{
		ModuleRecord();
		~ModuleRecord();
		ModuleRecord(ModuleRecord &&) noexcept;
		ModuleRecord &operator=(ModuleRecord &&) noexcept;
		ModuleRecord(const ModuleRecord &) = delete;
		ModuleRecord &operator=(const ModuleRecord &) = delete;

		ModuleHandle id = INVALID_MODULE_HANDLE;
		std::string name;
		std::uint32_t abiVersion = 0;
		EModuleKind kind = EModuleKind::NativeCpp;
		EModuleState state = EModuleState::Registering;

		/// Native library kept alive while descriptors and callbacks are registered.
		std::shared_ptr<SharedLibrary> library;

		/// Foreign language callbacks, valid when hasRuntimeOps is set.
		HushSystemRuntimeOps runtimeOps{};
		bool hasRuntimeOps = false;

		std::vector<SystemDescriptor> nativeSystems;
		std::vector<ForeignSystemDescriptor> foreignSystems;
	};

	/// Copyable public view of a module record, safe to retain after a query.
	struct ModuleInfo
	{
		ModuleHandle id = INVALID_MODULE_HANDLE;
		std::string name;
		std::uint32_t abiVersion = 0;
		EModuleKind kind = EModuleKind::NativeCpp;
		EModuleState state = EModuleState::Registering;
		std::size_t nativeSystemCount = 0;
		std::size_t foreignSystemCount = 0;
	};

	/// Engine owned registry of gameplay modules.
	///
	/// Registration is transactional: a module first calls
	/// BeginModuleRegistration, registers its types and systems, and then the
	/// loader calls CommitModuleRegistration. AbortModuleRegistration rolls
	/// everything back, including the types added to the reflection database.
	class ModuleRegistry
	{
	public:
		enum class EError : std::uint8_t
		{
			None = 0,
			ModuleNotFound,
			InvalidState,
			EngineModule,
			SystemNotFound,
			DuplicateModule,
			DuplicateType,
		};

		explicit ModuleRegistry(Reflection::ReflectionDB &db);
		~ModuleRegistry();

		ModuleRegistry(const ModuleRegistry &) = delete;
		ModuleRegistry &operator=(const ModuleRegistry &) = delete;

		/// Starts the registration of a module. The returned handle owns every
		/// type the module registers from this point on.
		/// @param name Display name of the module.
		/// @param kind Runtime kind of the module.
		/// @param abiVersion ABI version the module was built against.
		Result<ModuleHandle, EError> BeginModuleRegistration(std::string_view name, EModuleKind kind,
															 std::uint32_t abiVersion);

		/// Marks the module as active so the engine can use it.
		EError CommitModuleRegistration(ModuleHandle module);

		/// Rolls back a module that failed to register. All reflected types
		/// it registered are removed from the database.
		void AbortModuleRegistration(ModuleHandle module);

		/// Stores the foreign language callback table of the module.
		EError SetSystemRuntimeOps(ModuleHandle module, const HushSystemRuntimeOps &ops);

		/// Registers a foreign language system into the module.
		EError RegisterForeignSystem(ModuleHandle module, ForeignSystemDescriptor descriptor);

		/// Registers the native systems of the module from their descriptors.
		/// This function is header inline on purpose: native module libraries
		/// call it through the generated module entry point, and they cannot
		/// link against the engine symbols of the host process.
		EError RegisterNativeSystems(ModuleHandle module, std::span<const SystemDescriptor> systems)
		{
			std::unique_lock lock(m_mutex);
			const auto it = m_modules.find(module);
			if (it == m_modules.end())
			{
				return EError::ModuleNotFound;
			}
			if (it->second.state != EModuleState::Registering)
			{
				return EError::InvalidState;
			}

			for (std::size_t i = 0; i < systems.size(); ++i)
			{
				const SystemDescriptor &system = systems[i];
				if (system.name == nullptr || system.create == nullptr || system.order > ISystem::MAX_ORDER ||
					Reflection::TypeId{Hashing::Fnv1a64(system.name)} != system.typeId)
				{
					return EError::InvalidState;
				}
				const bool duplicate =
					std::ranges::any_of(it->second.nativeSystems, [&system](const SystemDescriptor &registered) {
						return registered.typeId == system.typeId;
					});
				const bool duplicateInBatch =
					std::ranges::any_of(systems.first(i), [&system](const SystemDescriptor &registered) {
						return registered.typeId == system.typeId;
					});
				if (duplicate || duplicateInBatch)
				{
					return EError::DuplicateType;
				}
			}

			it->second.nativeSystems.insert(it->second.nativeSystems.end(), systems.begin(), systems.end());
			return EError::None;
		}

		/// Creates a system instance of the given module and type.
		/// Native systems are created directly, foreign systems are wrapped in
		/// a ModuleSystem adapter.
		Result<std::unique_ptr<ISystem>, EError> CreateSystem(ModuleHandle module, Reflection::TypeId typeId,
															  Scene &scene);

		/// Creates a system from the stable names stored in manifests and scene assets.
		Result<std::unique_ptr<ISystem>, EError> CreateSystem(std::string_view moduleName, std::string_view typeName,
															  Scene &scene);

		/// Creates a registered system and transfers it to the scene's owned
		/// system lane for scheduling and lifecycle management.
		EError AddSystemToScene(ModuleHandle module, Reflection::TypeId typeId, Scene &scene);

		/// Finds which module owns a system type.
		Result<ModuleHandle, EError> FindSystemModule(Reflection::TypeId typeId);

		/// Finds a module by its display name.
		Result<ModuleHandle, EError> FindModuleByName(std::string_view name);

		/// Removes the module and every reflected type it owns. Native module
		/// libraries remain loaded until their last live system is destroyed.
		void UnregisterModule(ModuleHandle module);

		/// Gets a stable snapshot of a module record.
		std::optional<ModuleInfo> GetModuleInfo(ModuleHandle module) const;

		/// Transfers ownership of a native module library to its registry record.
		EError SetLibrary(ModuleHandle module, SharedLibrary library);

		/// The reflection database every module registers its types into.
		[[nodiscard]]
		Reflection::ReflectionDB &GetReflectionDB() const
		{
			return m_reflectionDB;
		}

	private:
		Reflection::ReflectionDB &m_reflectionDB;

		mutable std::shared_mutex m_mutex;
		std::unordered_map<ModuleHandle, ModuleRecord> m_modules;
		ModuleHandle m_nextHandle = ENGINE_MODULE_HANDLE + 1;
	};
} // namespace Hush::Modules
