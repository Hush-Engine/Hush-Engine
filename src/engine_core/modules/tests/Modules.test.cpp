/*! \file Modules.test.cpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Tests of the module registry and the native module loader
*/

#include "ModuleRegistry.hpp"
#include "ModuleSystem.hpp"
#include "NativeModuleEntry.hpp"
#include "NativeModuleLoader.hpp"

#include "Scene.hpp"

#include <catch2/catch_test_macros.hpp>

namespace
{
	// Test component registered by the handwritten native module.
	struct TestNativeComponent
	{
		int count = 0;
		float growth = 0.0F;
	};

	constexpr std::string_view TEST_COMPONENT_NAME = "Hush.Test.NativeComponent";
	constexpr std::string_view TEST_SYSTEM_NAME = "Hush.Test.NativeSystem";

	Hush::Reflection::TypeId TestComponentTypeId()
	{
		return Hush::Reflection::TypeId{Hush::Hashing::Fnv1a64(TEST_COMPONENT_NAME)};
	}

	Hush::Reflection::TypeId TestSystemTypeId()
	{
		return Hush::Reflection::TypeId{Hush::Hashing::Fnv1a64(TEST_SYSTEM_NAME)};
	}

	// Counters used to check that the foreign runtime callbacks are called.
	struct ForeignCalls
	{
		int created = 0;
		int destroyed = 0;
		int init = 0;
		int update = 0;
		int shutdown = 0;
		float lastDelta = 0.0F;
	};

	ForeignCalls g_foreignCalls;

	HushObjectHandle ForeignCreate(std::uint64_t, void *)
	{
		g_foreignCalls.created++;
		return HushObjectHandle{static_cast<uintptr_t>(0x1)};
	}

	void ForeignDestroy(HushObjectHandle)
	{
		g_foreignCalls.destroyed++;
	}

	void ForeignInit(HushObjectHandle)
	{
		g_foreignCalls.init++;
	}

	void ForeignUpdate(HushObjectHandle, float delta)
	{
		g_foreignCalls.update++;
		g_foreignCalls.lastDelta = delta;
	}

	void ForeignShutdown(HushObjectHandle)
	{
		g_foreignCalls.shutdown++;
	}

	Hush::ISystem *NullSystemFactory(Hush::Scene &)
	{
		return nullptr;
	}
} // namespace

TEST_CASE("Module registry tracks type ownership", "[modules]")
{
	Hush::Reflection::ReflectionDB db;
	Hush::Modules::ModuleRegistry registry(db);

	// The engine module always exists.
	REQUIRE(registry.GetModuleInfo(Hush::ENGINE_MODULE_HANDLE).has_value());

	auto module =
		registry.BeginModuleRegistration("TestModule", Hush::Modules::EModuleKind::NativeCpp, HUSH_MODULE_ABI_VERSION);
	REQUIRE(!module.has_error());

	Hush::Reflection::TypeInfo info(TestComponentTypeId());
	info.SetName(TEST_COMPONENT_NAME);
	info.SetSize(sizeof(TestNativeComponent));
	info.SetAlignment(alignof(TestNativeComponent));
	REQUIRE(db.RegisterClass(std::move(info), module.value()) == Hush::Reflection::ERegisterClassError::None);

	// Registering the same type again must fail instead of being ignored.
	Hush::Reflection::TypeInfo duplicate(TestComponentTypeId());
	duplicate.SetName(TEST_COMPONENT_NAME);
	REQUIRE(db.RegisterClass(std::move(duplicate), module.value()) ==
			Hush::Reflection::ERegisterClassError::DuplicateType);

	const Hush::Reflection::TypeInfo *registered = db.GetTypeInfo(TEST_COMPONENT_NAME);
	REQUIRE(registered != nullptr);
	REQUIRE(registered->GetOwner() == module.value());
	REQUIRE(!registered->IsBuiltin());
	REQUIRE(db.GetModuleTypes(module.value()).size() == 1);

	REQUIRE(registry.CommitModuleRegistration(module.value()) == Hush::Modules::ModuleRegistry::EError::None);

	registry.UnregisterModule(module.value());
	REQUIRE(db.GetTypeInfo(TEST_COMPONENT_NAME) == nullptr);
	REQUIRE_FALSE(registry.GetModuleInfo(module.value()).has_value());
}

TEST_CASE("Aborting a module removes its types", "[modules]")
{
	Hush::Reflection::ReflectionDB db;
	Hush::Modules::ModuleRegistry registry(db);

	auto module =
		registry.BeginModuleRegistration("AbortModule", Hush::Modules::EModuleKind::NativeCpp, HUSH_MODULE_ABI_VERSION);
	REQUIRE(!module.has_error());

	Hush::Reflection::TypeInfo info(TestComponentTypeId());
	info.SetName(TEST_COMPONENT_NAME);
	REQUIRE(db.RegisterClass(std::move(info), module.value()) == Hush::Reflection::ERegisterClassError::None);
	REQUIRE(db.GetTypeCount() == 1);

	registry.AbortModuleRegistration(module.value());
	REQUIRE(db.GetTypeCount() == 0);
	REQUIRE(db.GetTypeInfo(TEST_COMPONENT_NAME) == nullptr);
}

TEST_CASE("Native system descriptor batches accumulate", "[modules]")
{
	Hush::Reflection::ReflectionDB db;
	Hush::Modules::ModuleRegistry registry(db);

	constexpr std::string_view firstName = "Test.FirstSystem";
	constexpr std::string_view secondName = "Test.SecondSystem";
	const Hush::SystemDescriptor first{Hush::Reflection::TypeId{Hush::Hashing::Fnv1a64(firstName)}, firstName.data(), 1,
									   &NullSystemFactory};
	const Hush::SystemDescriptor second{Hush::Reflection::TypeId{Hush::Hashing::Fnv1a64(secondName)}, secondName.data(),
										2, &NullSystemFactory};
	REQUIRE(registry.RegisterNativeSystems(Hush::ENGINE_MODULE_HANDLE, std::span(&first, 1)) ==
			Hush::Modules::ModuleRegistry::EError::None);
	REQUIRE(registry.RegisterNativeSystems(Hush::ENGINE_MODULE_HANDLE, std::span(&second, 1)) ==
			Hush::Modules::ModuleRegistry::EError::None);

	const std::optional<Hush::Modules::ModuleInfo> engineModule = registry.GetModuleInfo(Hush::ENGINE_MODULE_HANDLE);
	REQUIRE(engineModule.has_value());
	REQUIRE(engineModule->nativeSystemCount == 2);
	REQUIRE(registry.RegisterNativeSystems(Hush::ENGINE_MODULE_HANDLE, std::span(&first, 1)) ==
			Hush::Modules::ModuleRegistry::EError::DuplicateType);

	constexpr std::string_view thirdName = "Test.ThirdSystem";
	const Hush::SystemDescriptor third{Hush::Reflection::TypeId{Hush::Hashing::Fnv1a64(thirdName)}, thirdName.data(), 3,
									   &NullSystemFactory};
	const Hush::SystemDescriptor duplicateBatch[] = {third, third};
	REQUIRE(registry.RegisterNativeSystems(Hush::ENGINE_MODULE_HANDLE, duplicateBatch) ==
			Hush::Modules::ModuleRegistry::EError::DuplicateType);
	const Hush::SystemDescriptor invalid{Hush::Reflection::TypeId{Hush::Hashing::Fnv1a64(thirdName)}, thirdName.data(),
										 3, nullptr};
	REQUIRE(registry.RegisterNativeSystems(Hush::ENGINE_MODULE_HANDLE, std::span(&invalid, 1)) ==
			Hush::Modules::ModuleRegistry::EError::InvalidState);
}

TEST_CASE("Native factories returning null fail cleanly", "[modules]")
{
	Hush::Reflection::ReflectionDB db;
	Hush::Modules::ModuleRegistry registry(db);
	const Hush::SystemDescriptor descriptor{TestSystemTypeId(), TEST_SYSTEM_NAME.data(), 0, &NullSystemFactory};
	REQUIRE(registry.RegisterNativeSystems(Hush::ENGINE_MODULE_HANDLE, std::span(&descriptor, 1)) ==
			Hush::Modules::ModuleRegistry::EError::None);
	REQUIRE(registry.CommitModuleRegistration(Hush::ENGINE_MODULE_HANDLE) ==
			Hush::Modules::ModuleRegistry::EError::None);

	Hush::Scene scene(nullptr, nullptr);
	auto system = registry.CreateSystem(Hush::ENGINE_MODULE_HANDLE, TestSystemTypeId(), scene);
	REQUIRE(system.has_error());
	REQUIRE(system.error() == Hush::Modules::ModuleRegistry::EError::SystemNotFound);
}

TEST_CASE("Foreign registration validates descriptors and runtime callbacks", "[modules]")
{
	Hush::Reflection::ReflectionDB db;
	Hush::Modules::ModuleRegistry registry(db);
	auto module =
		registry.BeginModuleRegistration("ForeignModule", Hush::Modules::EModuleKind::Rust, HUSH_MODULE_ABI_VERSION);
	REQUIRE(!module.has_error());

	Hush::Modules::ForeignSystemDescriptor invalidDescriptor;
	invalidDescriptor.typeId = Hush::Reflection::TypeId{1};
	invalidDescriptor.name = std::string(TEST_SYSTEM_NAME);
	REQUIRE(registry.RegisterForeignSystem(module.value(), std::move(invalidDescriptor)) ==
			Hush::Modules::ModuleRegistry::EError::InvalidState);

	Hush::Modules::ForeignSystemDescriptor descriptor;
	descriptor.typeId = TestSystemTypeId();
	descriptor.name = std::string(TEST_SYSTEM_NAME);
	REQUIRE(registry.RegisterForeignSystem(module.value(), std::move(descriptor)) ==
			Hush::Modules::ModuleRegistry::EError::None);
	REQUIRE(registry.CommitModuleRegistration(module.value()) == Hush::Modules::ModuleRegistry::EError::InvalidState);

	HushSystemRuntimeOps invalidOps{};
	REQUIRE(registry.SetSystemRuntimeOps(module.value(), invalidOps) ==
			Hush::Modules::ModuleRegistry::EError::InvalidState);
	HushSystemRuntimeOps ops{};
	ops.create = &ForeignCreate;
	ops.destroy = &ForeignDestroy;
	REQUIRE(registry.SetSystemRuntimeOps(module.value(), ops) == Hush::Modules::ModuleRegistry::EError::None);
	REQUIRE(registry.CommitModuleRegistration(module.value()) == Hush::Modules::ModuleRegistry::EError::None);
}

TEST_CASE("Foreign systems run through the module adapter", "[modules]")
{
	Hush::Reflection::ReflectionDB db;
	Hush::Modules::ModuleRegistry registry(db);
	g_foreignCalls = {};

	auto module = registry.BeginModuleRegistration("ForeignModule", Hush::Modules::EModuleKind::DotNetCoreClr,
												   HUSH_MODULE_ABI_VERSION);
	REQUIRE(!module.has_error());

	HushSystemRuntimeOps ops{};
	ops.create = &ForeignCreate;
	ops.destroy = &ForeignDestroy;
	ops.init = &ForeignInit;
	ops.update = &ForeignUpdate;
	ops.shutdown = &ForeignShutdown;
	REQUIRE(registry.SetSystemRuntimeOps(module.value(), ops) == Hush::Modules::ModuleRegistry::EError::None);

	Hush::Modules::ForeignSystemDescriptor descriptor;
	descriptor.typeId = TestSystemTypeId();
	descriptor.name = std::string(TEST_SYSTEM_NAME);
	descriptor.order = 7;
	descriptor.lifecycleMask = HushSystemLifecycle_Init | HushSystemLifecycle_Update | HushSystemLifecycle_Shutdown;
	REQUIRE(registry.RegisterForeignSystem(module.value(), std::move(descriptor)) ==
			Hush::Modules::ModuleRegistry::EError::None);
	const Hush::Reflection::TypeInfo *typeInfo = db.GetTypeInfo(TEST_SYSTEM_NAME);
	REQUIRE(typeInfo != nullptr);
	REQUIRE(typeInfo->GetOwner() == module.value());
	REQUIRE(typeInfo->HasMetadata(Hush::Reflection::METADATA_KEY_SYSTEM));
	REQUIRE(registry.CommitModuleRegistration(module.value()) == Hush::Modules::ModuleRegistry::EError::None);

	auto foundModule = registry.FindSystemModule(TestSystemTypeId());
	REQUIRE(!foundModule.has_error());
	REQUIRE(foundModule.value() == module.value());

	Hush::Scene scene(nullptr, nullptr);
	{
		auto system = registry.CreateSystem(module.value(), TestSystemTypeId(), scene);
		REQUIRE(!system.has_error());
		REQUIRE(system.value()->GetName() == TEST_SYSTEM_NAME);
		REQUIRE(system.value()->Order() == 7);
		REQUIRE(g_foreignCalls.created == 1);

		system.value()->Init();
		system.value()->OnUpdate(0.5F);
		system.value()->OnFixedUpdate(0.5F); // not in the mask, must not crash
		system.value()->OnShutdown();

		REQUIRE(g_foreignCalls.init == 1);
		REQUIRE(g_foreignCalls.update == 1);
		REQUIRE(g_foreignCalls.lastDelta == 0.5F);
		REQUIRE(g_foreignCalls.shutdown == 1);
		REQUIRE(g_foreignCalls.destroyed == 0);
	}

	// Destroying the system must destroy the foreign object.
	REQUIRE(g_foreignCalls.destroyed == 1);
	REQUIRE(registry.AddSystemToScene(module.value(), TestSystemTypeId(), scene) ==
			Hush::Modules::ModuleRegistry::EError::None);
	REQUIRE(g_foreignCalls.created == 2);
	std::string sceneAsset;
	REQUIRE(scene.ToSceneAsset(sceneAsset) == Hush::Scene::EError::None);
	REQUIRE(sceneAsset.find("ForeignModule") != std::string::npos);
	REQUIRE(sceneAsset.find(TEST_SYSTEM_NAME) != std::string::npos);
	scene.RemoveSystem(TEST_SYSTEM_NAME);
	REQUIRE(g_foreignCalls.destroyed == 2);
	scene.SetSystemFactory([&registry](Hush::Scene &targetScene, std::string_view moduleName,
									   std::string_view typeName) -> std::unique_ptr<Hush::ISystem> {
		auto system = registry.CreateSystem(moduleName, typeName, targetScene);
		return system.has_error() ? nullptr : std::move(system.value());
	});
	REQUIRE(scene.FromSceneAsset(sceneAsset) == Hush::Scene::EError::None);
	REQUIRE(g_foreignCalls.created == 3);
	scene.RemoveSystem(TEST_SYSTEM_NAME);
	REQUIRE(g_foreignCalls.destroyed == 3);

	registry.UnregisterModule(module.value());
	REQUIRE(db.GetTypeInfo(TEST_SYSTEM_NAME) == nullptr);
}

TEST_CASE("Handwritten native module loads and registers", "[modules]")
{
	Hush::Reflection::ReflectionDB db;
	Hush::Modules::ModuleRegistry registry(db);
	Hush::Modules::NativeModuleLoader loader(registry, nullptr, nullptr);

	auto module = loader.Load(HUSH_TEST_MODULE_NAME, "ManifestModule", Hush::Modules::EModuleKind::Rust);
	REQUIRE(!module.has_error());
	const std::optional<Hush::Modules::ModuleInfo> record = registry.GetModuleInfo(module.value());
	REQUIRE(record.has_value());
	REQUIRE(record->name == "ManifestModule");
	REQUIRE(record->kind == Hush::Modules::EModuleKind::Rust);

	// The module registered its component and owns it.
	const Hush::Reflection::TypeInfo *info = db.GetTypeInfo(TEST_COMPONENT_NAME);
	REQUIRE(info != nullptr);
	REQUIRE(info->GetOwner() == module.value());
	REQUIRE(info->GetSize() == sizeof(TestNativeComponent));

	// The module registered its system and the scene can create it.
	Hush::Scene scene(nullptr, nullptr);
	auto system = registry.CreateSystem(module.value(), TestSystemTypeId(), scene);
	REQUIRE(!system.has_error());
	REQUIRE(system.value()->GetName() == TEST_SYSTEM_NAME);
	REQUIRE(system.value()->Order() == 3);

	system.value()->Init();
	system.value()->OnUpdate(1.0F);
	system.value()->OnShutdown();

	// Unregistering removes reflected data immediately, while a live system keeps
	// the native library loaded until its own destruction.
	registry.UnregisterModule(module.value());
	REQUIRE(db.GetTypeInfo(TEST_COMPONENT_NAME) == nullptr);
	REQUIRE(system.value()->GetName() == TEST_SYSTEM_NAME);
	system.value().reset();
}

TEST_CASE("Generated native module registers a reflected system", "[modules]")
{
	Hush::Reflection::ReflectionDB db;
	Hush::Modules::ModuleRegistry registry(db);
	Hush::Modules::NativeModuleLoader loader(registry, nullptr, nullptr);

	auto module = loader.Load(HUSH_GENERATED_TEST_MODULE_NAME);
	REQUIRE(!module.has_error());

	constexpr std::string_view systemName = "Hush.Tests.GeneratedModuleSystem";
	const Hush::Reflection::TypeId systemType{Hush::Hashing::Fnv1a64(systemName)};
	const Hush::Reflection::TypeInfo *type = db.GetTypeInfo(systemType);
	REQUIRE(type != nullptr);
	REQUIRE(type->GetOwner() == module.value());
	REQUIRE(type->GetName() == systemName);

	Hush::Scene scene(nullptr, nullptr);
	auto system = registry.CreateSystem(module.value(), systemType, scene);
	REQUIRE(!system.has_error());
	REQUIRE(system.value()->GetName() == systemName);
	REQUIRE(system.value()->Order() == 17);
}

TEST_CASE("Missing module library fails cleanly", "[modules]")
{
	Hush::Reflection::ReflectionDB db;
	Hush::Modules::ModuleRegistry registry(db);
	Hush::Modules::NativeModuleLoader loader(registry, nullptr, nullptr);

	auto module = loader.Load("this-module-does-not-exist.dll");
	REQUIRE(module.has_error());
	REQUIRE(module.error() == Hush::Modules::NativeModuleLoader::EError::LibraryLoadFailed);
}
