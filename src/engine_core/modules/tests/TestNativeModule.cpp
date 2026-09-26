/*! \file TestNativeModule.cpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Handwritten native module used by the module loader tests

	This file shows what a native C++ gameplay module looks like. The
	reflection generator emits this same shape for generated modules.
*/

#include "HushModuleAbi.h"
#include "NativeModuleEntry.hpp"

#include "ISystem.hpp"
#include "SystemDescriptor.hpp"

#include <crypto/Hashing.hpp>

namespace
{
	struct TestNativeComponent
	{
		int count = 0;
		float growth = 0.0F;
	};

	constexpr std::string_view TEST_COMPONENT_NAME = "Hush.Test.NativeComponent";
	constexpr std::string_view TEST_SYSTEM_NAME = "Hush.Test.NativeSystem";

	class TestNativeSystem final : public Hush::ISystem
	{
	public:
		explicit TestNativeSystem(Hush::Scene &scene)
			: ISystem(scene)
		{
			SetOrder(3);
		}

		void Init() override
		{
		}

		void OnUpdate(float) override
		{
		}

		void OnFixedUpdate(float) override
		{
		}

		void OnShutdown() override
		{
		}

		void OnPreRender() override
		{
		}

		void OnRender() override
		{
		}

		void OnPostRender() override
		{
		}

		[[nodiscard]]
		std::string_view GetName() const override
		{
			return TEST_SYSTEM_NAME;
		}
	};

	Hush::ISystem *CreateTestNativeSystem(Hush::Scene &scene)
	{
		return new TestNativeSystem(scene);
	}

	bool RegisterTestTypes(Hush::Reflection::ReflectionDB &db, Hush::ModuleHandle module)
	{
		Hush::Reflection::TypeInfo info(Hush::Reflection::TypeId{Hush::Hashing::Fnv1a64(TEST_COMPONENT_NAME)});
		info.SetName(TEST_COMPONENT_NAME);
		info.SetSize(sizeof(TestNativeComponent));
		info.SetAlignment(alignof(TestNativeComponent));

		return db.RegisterClass(std::move(info), module) == Hush::Reflection::ERegisterClassError::None;
	}

	const Hush::SystemDescriptor TEST_SYSTEMS[] = {
		{Hush::Reflection::TypeId{Hush::Hashing::Fnv1a64(TEST_SYSTEM_NAME)}, TEST_SYSTEM_NAME.data(), 3,
		 &CreateTestNativeSystem},
	};
} // namespace

extern "C" HUSH_MODULE_EXPORT HushModuleResult HushRegisterModule(const HushModuleContext *context)
{
	return Hush::Modules::NativeModuleEntry(context, &RegisterTestTypes, TEST_SYSTEMS);
}
