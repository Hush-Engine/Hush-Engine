/*! \file ModuleRegistry.cpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Registry of the gameplay modules loaded by the engine
*/

#include "ModuleRegistry.hpp"

#include "Logger.hpp"
#include "ModuleSystem.hpp"
#include "Scene.hpp"
#include "SharedLibrary.hpp"

#include <optional>

Hush::Modules::ModuleRecord::ModuleRecord() = default;
Hush::Modules::ModuleRecord::~ModuleRecord() = default;
Hush::Modules::ModuleRecord::ModuleRecord(ModuleRecord &&) noexcept = default;
Hush::Modules::ModuleRecord &Hush::Modules::ModuleRecord::operator=(ModuleRecord &&) noexcept = default;

namespace
{
	/// Keeps a native module loaded until its last system instance is destroyed.
	class LeasedSystem final : public Hush::ISystem
	{
	public:
		LeasedSystem(Hush::Scene &scene, std::unique_ptr<Hush::ISystem> system,
					 std::shared_ptr<Hush::SharedLibrary> library)
			: ISystem(scene),
			  m_library(std::move(library)),
			  m_system(std::move(system))
		{
			SetOrder(m_system->Order());
		}

		void Init() override
		{
			m_system->Init();
		}
		void OnShutdown() override
		{
			m_system->OnShutdown();
		}
		void OnUpdate(float delta) override
		{
			m_system->OnUpdate(delta);
		}
		void OnFixedUpdate(float delta) override
		{
			m_system->OnFixedUpdate(delta);
		}
		void OnRender() override
		{
			m_system->OnRender();
		}
		void OnPreRender() override
		{
			m_system->OnPreRender();
		}
		void OnPostRender() override
		{
			m_system->OnPostRender();
		}

		std::string_view GetName() const override
		{
			return m_system->GetName();
		}

	private:
		// Declaration order ensures the implementation dies before its library lease.
		std::shared_ptr<Hush::SharedLibrary> m_library;
		std::unique_ptr<Hush::ISystem> m_system;
	};

	std::unique_ptr<Hush::ISystem> LeaseSystem(Hush::Scene &scene, std::unique_ptr<Hush::ISystem> system,
											   std::shared_ptr<Hush::SharedLibrary> library)
	{
		if (library == nullptr)
		{
			return system;
		}
		return std::make_unique<LeasedSystem>(scene, std::move(system), std::move(library));
	}
} // namespace

Hush::Modules::ModuleRegistry::ModuleRegistry(Reflection::ReflectionDB &db)
	: m_reflectionDB(db)
{
	// The engine itself is always the first module so built-in types have an
	// owner too. It stays in the registering state until the engine finishes
	// registering the built-in types. The engine module is never unregistered.
	ModuleRecord engineModule;
	engineModule.id = ENGINE_MODULE_HANDLE;
	engineModule.name = "Hush";
	engineModule.abiVersion = HUSH_MODULE_ABI_VERSION;
	engineModule.kind = EModuleKind::NativeCpp;
	engineModule.state = EModuleState::Registering;
	m_modules.emplace(ENGINE_MODULE_HANDLE, std::move(engineModule));
}

Hush::Modules::ModuleRegistry::~ModuleRegistry()
{
	std::unordered_map<ModuleHandle, ModuleRecord> modules;
	{
		std::unique_lock lock(m_mutex);
		modules.swap(m_modules);
	}

	for (const auto &[handle, record] : modules)
	{
		if (handle != ENGINE_MODULE_HANDLE)
		{
			m_reflectionDB.UnregisterModule(handle);
		}
	}
}

Hush::Result<Hush::ModuleHandle, Hush::Modules::ModuleRegistry::EError> Hush::Modules::ModuleRegistry::
	BeginModuleRegistration(std::string_view name, EModuleKind kind, std::uint32_t abiVersion)
{
	std::unique_lock lock(m_mutex);
	if (name.empty() || std::ranges::any_of(m_modules, [name](const auto &entry) { return entry.second.name == name; }))
	{
		return name.empty() ? EError::InvalidState : EError::DuplicateModule;
	}

	ModuleHandle handle = m_nextHandle++;

	ModuleRecord record;
	record.id = handle;
	record.name = std::string(name);
	record.abiVersion = abiVersion;
	record.kind = kind;
	record.state = EModuleState::Registering;

	m_modules.emplace(handle, std::move(record));
	return handle;
}

Hush::Modules::ModuleRegistry::EError Hush::Modules::ModuleRegistry::CommitModuleRegistration(ModuleHandle module)
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
	if (!it->second.foreignSystems.empty() && !it->second.hasRuntimeOps)
	{
		return EError::InvalidState;
	}

	it->second.state = EModuleState::Active;
	return EError::None;
}

void Hush::Modules::ModuleRegistry::AbortModuleRegistration(ModuleHandle module)
{
	// Remove the reflected types first so no dangling type is left behind.
	m_reflectionDB.UnregisterModule(module);

	std::shared_ptr<SharedLibrary> library;
	{
		std::unique_lock lock(m_mutex);
		const auto it = m_modules.find(module);
		if (it != m_modules.end())
		{
			library = std::move(it->second.library);
			m_modules.erase(it);
		}
	}
}

Hush::Modules::ModuleRegistry::EError Hush::Modules::ModuleRegistry::SetSystemRuntimeOps(
	ModuleHandle module, const HushSystemRuntimeOps &ops)
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
	if (ops.create == nullptr || ops.destroy == nullptr)
	{
		return EError::InvalidState;
	}

	it->second.runtimeOps = ops;
	it->second.hasRuntimeOps = true;
	return EError::None;
}

Hush::Modules::ModuleRegistry::EError Hush::Modules::ModuleRegistry::RegisterForeignSystem(
	ModuleHandle module, ForeignSystemDescriptor descriptor)
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
	if (descriptor.name.empty() || descriptor.order > ISystem::MAX_ORDER ||
		Reflection::TypeId{Hashing::Fnv1a64(descriptor.name)} != descriptor.typeId)
	{
		return EError::InvalidState;
	}

	Reflection::TypeInfo typeInfo(descriptor.typeId);
	typeInfo.SetName(descriptor.name);
	typeInfo.AddMetadata(Reflection::METADATA_KEY_SYSTEM.data(), "true");
	if (m_reflectionDB.RegisterClass(std::move(typeInfo), module) != Reflection::ERegisterClassError::None)
	{
		return EError::DuplicateType;
	}

	it->second.foreignSystems.push_back(std::move(descriptor));
	return EError::None;
}

Hush::Result<std::unique_ptr<Hush::ISystem>, Hush::Modules::ModuleRegistry::EError> Hush::Modules::ModuleRegistry::
	CreateSystem(ModuleHandle module, Reflection::TypeId typeId, Scene &scene)
{
	std::optional<SystemDescriptor> nativeDescriptor;
	std::optional<ForeignSystemDescriptor> foreignDescriptor;
	HushSystemRuntimeOps runtimeOps{};
	std::shared_ptr<SharedLibrary> library;
	{
		std::shared_lock lock(m_mutex);
		const auto it = m_modules.find(module);
		if (it == m_modules.end())
		{
			return EError::ModuleNotFound;
		}
		const ModuleRecord &record = it->second;
		if (record.state != EModuleState::Active)
		{
			return EError::InvalidState;
		}
		library = record.library;

		for (const SystemDescriptor &descriptor : record.nativeSystems)
		{
			if (descriptor.typeId == typeId)
			{
				nativeDescriptor = descriptor;
				break;
			}
		}
		if (!nativeDescriptor.has_value())
		{
			for (const ForeignSystemDescriptor &descriptor : record.foreignSystems)
			{
				if (descriptor.typeId == typeId)
				{
					foreignDescriptor = descriptor;
					break;
				}
			}
		}
		if (foreignDescriptor.has_value())
		{
			if (!record.hasRuntimeOps || record.runtimeOps.create == nullptr)
			{
				return EError::InvalidState;
			}
			runtimeOps = record.runtimeOps;
		}
	}

	if (nativeDescriptor.has_value())
	{
		std::unique_ptr<ISystem> system(nativeDescriptor->create(scene));
		if (system == nullptr)
		{
			return EError::SystemNotFound;
		}
		return LeaseSystem(scene, std::move(system), std::move(library));
	}

	if (foreignDescriptor.has_value())
	{
		HushObjectHandle object = runtimeOps.create(typeId.id, &scene);
		if (object.value == 0)
		{
			return EError::SystemNotFound;
		}

		return LeaseSystem(scene,
						   std::make_unique<ModuleSystem>(scene, module, object, runtimeOps, foreignDescriptor->name,
														  foreignDescriptor->order, foreignDescriptor->lifecycleMask),
						   std::move(library));
	}

	return EError::SystemNotFound;
}

Hush::Result<std::unique_ptr<Hush::ISystem>, Hush::Modules::ModuleRegistry::EError> Hush::Modules::ModuleRegistry::
	CreateSystem(std::string_view moduleName, std::string_view typeName, Scene &scene)
{
	if (typeName.empty())
	{
		return EError::SystemNotFound;
	}

	const Reflection::TypeId typeId{Hashing::Fnv1a64(typeName)};
	Result<ModuleHandle, EError> module = moduleName.empty() ? FindSystemModule(typeId) : FindModuleByName(moduleName);
	if (module.has_error())
	{
		return module.error();
	}

	{
		std::shared_lock lock(m_mutex);
		const auto record = m_modules.find(module.value());
		if (record == m_modules.end() || record->second.state != EModuleState::Active)
		{
			return EError::ModuleNotFound;
		}
		const bool nativeMatch = std::ranges::any_of(record->second.nativeSystems, [&](const SystemDescriptor &system) {
			return system.typeId == typeId && std::string_view(system.name) == typeName;
		});
		const bool foreignMatch =
			std::ranges::any_of(record->second.foreignSystems, [&](const ForeignSystemDescriptor &system) {
				return system.typeId == typeId && system.name == typeName;
			});
		if (!nativeMatch && !foreignMatch)
		{
			return EError::SystemNotFound;
		}
	}

	return CreateSystem(module.value(), typeId, scene);
}

Hush::Modules::ModuleRegistry::EError Hush::Modules::ModuleRegistry::AddSystemToScene(ModuleHandle module,
																					  Reflection::TypeId typeId,
																					  Scene &scene)
{
	std::string moduleName;
	std::string typeName;
	{
		std::shared_lock lock(m_mutex);
		const auto record = m_modules.find(module);
		if (record == m_modules.end())
		{
			return EError::ModuleNotFound;
		}
		moduleName = record->second.name;
		for (const SystemDescriptor &descriptor : record->second.nativeSystems)
		{
			if (descriptor.typeId == typeId)
			{
				typeName = descriptor.name;
				break;
			}
		}
		if (typeName.empty())
		{
			for (const ForeignSystemDescriptor &descriptor : record->second.foreignSystems)
			{
				if (descriptor.typeId == typeId)
				{
					typeName = descriptor.name;
					break;
				}
			}
		}
	}
	if (typeName.empty())
	{
		return EError::SystemNotFound;
	}

	Result<std::unique_ptr<ISystem>, EError> system = CreateSystem(module, typeId, scene);
	if (system.has_error())
	{
		return system.error();
	}

	scene.AddSystem(std::move(system.value()),
					SerializedSystem{.module = std::move(moduleName), .type = std::move(typeName)});
	return EError::None;
}

Hush::Result<Hush::ModuleHandle, Hush::Modules::ModuleRegistry::EError> Hush::Modules::ModuleRegistry::FindSystemModule(
	Reflection::TypeId typeId)
{
	std::shared_lock lock(m_mutex);
	for (const auto &[handle, record] : m_modules)
	{
		if (record.state != EModuleState::Active)
		{
			continue;
		}
		for (const SystemDescriptor &descriptor : record.nativeSystems)
		{
			if (descriptor.typeId == typeId)
			{
				return handle;
			}
		}
		for (const ForeignSystemDescriptor &descriptor : record.foreignSystems)
		{
			if (descriptor.typeId == typeId)
			{
				return handle;
			}
		}
	}
	return EError::SystemNotFound;
}

Hush::Result<Hush::ModuleHandle, Hush::Modules::ModuleRegistry::EError> Hush::Modules::ModuleRegistry::FindModuleByName(
	std::string_view name)
{
	std::shared_lock lock(m_mutex);
	for (const auto &[handle, record] : m_modules)
	{
		if (record.state == EModuleState::Active && record.name == name)
		{
			return handle;
		}
	}
	return EError::ModuleNotFound;
}

void Hush::Modules::ModuleRegistry::UnregisterModule(ModuleHandle module)
{
	if (module == ENGINE_MODULE_HANDLE)
	{
		LogFormat(ELogLevel::Warn, "The engine module cannot be unregistered");
		return;
	}

	// Remove the reflected types first, their callbacks may point into module
	// code that is about to be unloaded.
	m_reflectionDB.UnregisterModule(module);

	std::shared_ptr<SharedLibrary> library;
	{
		std::unique_lock lock(m_mutex);
		const auto it = m_modules.find(module);
		if (it != m_modules.end())
		{
			library = std::move(it->second.library);
			m_modules.erase(it);
		}
	}
}

std::optional<Hush::Modules::ModuleInfo> Hush::Modules::ModuleRegistry::GetModuleInfo(ModuleHandle module) const
{
	std::shared_lock lock(m_mutex);
	const auto it = m_modules.find(module);
	if (it == m_modules.end())
	{
		return std::nullopt;
	}
	const ModuleRecord &record = it->second;
	return ModuleInfo{.id = record.id,
					  .name = record.name,
					  .abiVersion = record.abiVersion,
					  .kind = record.kind,
					  .state = record.state,
					  .nativeSystemCount = record.nativeSystems.size(),
					  .foreignSystemCount = record.foreignSystems.size()};
}

Hush::Modules::ModuleRegistry::EError Hush::Modules::ModuleRegistry::SetLibrary(ModuleHandle module,
																				SharedLibrary library)
{
	std::unique_lock lock(m_mutex);
	const auto it = m_modules.find(module);
	if (it == m_modules.end())
	{
		return EError::ModuleNotFound;
	}
	if (it->second.state != EModuleState::Registering || it->second.library != nullptr)
	{
		return EError::InvalidState;
	}
	it->second.library = std::make_shared<SharedLibrary>(std::move(library));
	return EError::None;
}
