/*! \file Reflection.test.cpp
	\author Alan Ramirez
	\date 2025-05-25
	\brief Reflection test implementation
*/

#include <reflection/Type.hpp>
#include <catch2/catch_test_macros.hpp>
#include "Reflection.test.hpp"

struct MyTest
{
	int32_t a{0};

public:
	static constexpr std::uint64_t TypeId()
	{
		return Hush::Hashing::Fnv1a64(TypeName());
	}

	static constexpr std::string_view TypeName()
	{
		return "MyTest";
	}

	void MyFunc()
	{
		// Just a dummy function to test reflection
	}

	static void RegisterReflection(Hush::Reflection::ReflectionDB &db)
	{
		Hush::Reflection::FieldInfo::Getter aGetter = [](std::span<const Hush::Reflection::VariantView> params)
			-> Hush::Result<Hush::Reflection::Variant, Hush::Reflection::Variant::EVariantError> {
			if (params.size() != 1)
			{
				return Hush::Reflection::Variant::EVariantError::NonSameType;
			}
			auto result = params[0].Get<MyTest>();
			if (result.has_error())
			{
				return result.error();
			}

			MyTest *instance = result.value();

			return Hush::Reflection::Variant(instance->a);
		};

		Hush::Reflection::FieldInfo::Setter aSetter =
			[](std::span<const Hush::Reflection::VariantView> params) -> Hush::Reflection::Variant::EVariantError {
			if (params.size() != 2)
			{
				return Hush::Reflection::Variant::EVariantError::NonSameType;
			}
			auto result = params[0].Get<MyTest>();
			if (result.has_error())
			{
				return result.error();
			}

			MyTest *instance = result.value();

			auto value = params[1].Get<int32_t>();
			if (value.has_error())
			{
				return value.error();
			}

			instance->a = *value.value();

			return {};
		};

		db.RegisterClass<MyTest>()
			.AddConstructor(Hush::Reflection::FunctionInfo::Create(
				[](std::span<const Hush::Reflection::VariantView> params)
					-> Hush::Result<Hush::Reflection::Variant, Hush::Reflection::FunctionInfo::EFunctionInfoError> {
					if (!params.empty())
					{
						return Hush::Reflection::FunctionInfo::EFunctionInfoError::InvalidArgsCount;
					}

					return Hush::Reflection::Variant(MyTest{});
				},
				"MyTest"))
			.AddInPlaceConstructor(Hush::Reflection::TypeInfo::InPlaceCtor::Create(
				[](void *mem, std::span<const Hush::Reflection::VariantView> args) {
					if (args.size() != 0)
					{
						return Hush::Reflection::TypeInfo::EInPlaceConstructorError::NonMatchingArgs;
					}

					std::construct_at(static_cast<MyTest *>(mem));
					return Hush::Reflection::TypeInfo::EInPlaceConstructorError::None;
				}))
			.AddFunction(Hush::Reflection::FunctionInfo::Create<MyTest>(
				[](std::span<const Hush::Reflection::VariantView> params)
					-> Hush::Result<Hush::Reflection::Variant, Hush::Reflection::FunctionInfo::EFunctionInfoError> {
					if (params.size() != 1)
					{
						return Hush::Reflection::FunctionInfo::EFunctionInfoError::InvalidArgsCount;
					}

					MyTest *instance = params[0].Get<MyTest>().value();

					instance->MyFunc();

					return Hush::Reflection::Variant();
				},
				"MyFunc"))
			.AddProperty(Hush::Reflection::FieldInfo(Hush::Reflection::GetTypeId<int>(), "a", aSetter, aGetter,
													 offsetof(MyTest, a)))
			.Register();
	}
};

TEST_CASE("Reflection", "[reflection]")
{
	SECTION("Get/Set field reflection")
	{
		Hush::Reflection::ReflectionDB reflectionDB;
		MyTest::RegisterReflection(reflectionDB);

		auto typeInfo = reflectionDB.GetTypeInfo("MyTest");

		REQUIRE(typeInfo != nullptr);

		REQUIRE(typeInfo->GetFields().size() == 1);
		REQUIRE(typeInfo->GetFields()[0].GetName() == "a");
		REQUIRE(typeInfo->GetFields()[0].GetTypeId() == Hush::Reflection::GetTypeId<int>());

		MyTest test;
		test.a = 10;
		int32_t value = 0;

		const Hush::Reflection::FieldInfo &fieldInfo = typeInfo->GetFields()[0];

		(void)fieldInfo.Set({Hush::Reflection::VariantView(&test), Hush::Reflection::VariantView(&value)});

		REQUIRE(test.a == 0);
	}

	SECTION("Call function")
	{
		Hush::Reflection::ReflectionDB reflectionDB;
		MyTest::RegisterReflection(reflectionDB);

		const Hush::Reflection::TypeInfo *typeInfo = reflectionDB.GetTypeInfo("MyTest");

		REQUIRE(typeInfo != nullptr);

		REQUIRE(typeInfo->GetFunctions().size() == 1);
		REQUIRE(typeInfo->GetFunctions()[0].GetName() == "MyFunc");

		MyTest test;
		Hush::Result<Hush::Reflection::Variant, Hush::Reflection::FunctionInfo::EFunctionInfoError> result =
			typeInfo->CallFunction("MyFunc", {Hush::Reflection::VariantView(&test)});

		REQUIRE(!result.has_error());
		REQUIRE(result.value().IsType<void>());
	}

	SECTION("Construct class")
	{
		Hush::Reflection::ReflectionDB reflectionDB;
		MyTest::RegisterReflection(reflectionDB);

		auto typeInfo = reflectionDB.GetTypeInfo("MyTest");

		REQUIRE(typeInfo != nullptr);

		auto result = typeInfo->CreateInstance({});
		REQUIRE(result.has_value());

		Hush::Reflection::Variant variant = std::move(result.value());
		REQUIRE(variant.IsType<MyTest>());

		int value = 42;
		(void)typeInfo->GetFields()[0].Set({variant, Hush::Reflection::VariantView(&value)});

		auto instanceResult = variant.Get<MyTest>();
		REQUIRE(instanceResult.has_value());
		MyTest *instance = instanceResult.value();
		REQUIRE(instance != nullptr);
		REQUIRE(instance->a == 42);
	}

	SECTION("Autogen Reflection")
	{
		Hush::Reflection::ReflectionDB reflectionDB;

		AutogenTest::RegisterReflection(reflectionDB);

		auto typeInfo = reflectionDB.GetTypeInfo("AutogenTest");
		REQUIRE(typeInfo != nullptr);
		REQUIRE(typeInfo->GetFields().size() == 2);
		REQUIRE(typeInfo->GetFields()[0].GetName() == "myCustomField");
		REQUIRE(typeInfo->GetFields()[0].GetTypeId() == Hush::Reflection::GetTypeId<int>());
		REQUIRE(typeInfo->GetFields()[1].GetName() == "myFloatField");
		REQUIRE(typeInfo->GetFields()[1].GetTypeId() == Hush::Reflection::GetTypeId<float>());

		AutogenTest test;
		test.myCustomField = 42;
		test.myFloatField = 3.14f;

		auto customFieldInfo = typeInfo->GetField("myCustomField");
		REQUIRE(customFieldInfo.has_value());

		auto &field = customFieldInfo.value().get();

		int newValue = 100;
		(void)field.Set({Hush::Reflection::VariantView(&test), Hush::Reflection::VariantView(&newValue)});
		REQUIRE(test.myCustomField == newValue);

		// Get the float field
		auto floatFieldInfo = typeInfo->GetField("myFloatField");
		REQUIRE(floatFieldInfo.has_value());
		auto &floatField = floatFieldInfo.value().get();

		auto result = floatField.Get({Hush::Reflection::VariantView(&test)});
		REQUIRE(result.has_value());
		Hush::Result<float *, Hush::Reflection::VariantView::EVariantError> floatValue = result.value().Get<float>();
		REQUIRE(floatValue.has_value());
		REQUIRE(floatValue.value() != nullptr);
		REQUIRE(*floatValue.value() == test.myFloatField);
	}

	SECTION("Autogen reflection with function")
	{
		Hush::Reflection::ReflectionDB reflectionDB;

		AutogenTest::RegisterReflection(reflectionDB);

		auto typeInfo = reflectionDB.GetTypeInfo("AutogenTest");
		REQUIRE(typeInfo != nullptr);
		REQUIRE(typeInfo->GetFunctions().size() == 3);

		AutogenTest test;
		auto result = typeInfo->CallFunction("SetTo10", {Hush::Reflection::VariantView(&test)});
		REQUIRE(!result.has_error());
		REQUIRE(test.myCustomField == 10);

		int newValue = 20;
		auto getResult = typeInfo->CallFunction(
			"SetTo", {Hush::Reflection::VariantView(&test), Hush::Reflection::VariantView(&newValue)});
		REQUIRE(!getResult.has_error());
		REQUIRE(test.myCustomField == newValue);

		int outValue = 0;
		auto outResult = typeInfo->CallFunction(
			"GetFromRef", {Hush::Reflection::VariantView(&test), Hush::Reflection::VariantView(&outValue)});
		REQUIRE(!outResult.has_error());
		REQUIRE(outValue == test.myCustomField);
	}

	SECTION("Autogen reflection construct")
	{
		Hush::Reflection::ReflectionDB reflectionDB;
		AutogenTest::RegisterReflection(reflectionDB);

		auto typeInfo = reflectionDB.GetTypeInfo("AutogenTest");

		int value = 42;
		Hush::Result<Hush::Reflection::Variant, Hush::Reflection::FunctionInfo::EFunctionInfoError> newInstanceResult =
			typeInfo->CreateInstance({Hush::Reflection::VariantView(&value)});

		REQUIRE(newInstanceResult.has_value());
		Hush::Reflection::Variant newInstance = std::move(newInstanceResult.value());
		REQUIRE(newInstance.IsType<AutogenTest>());
		AutogenTest *instance = newInstance.Get<AutogenTest>().value();

		REQUIRE(instance->myCustomField == value);
	}

	SECTION("Autogen in place ctor")
	{
		Hush::Reflection::ReflectionDB reflectionDB;
		AutogenTest::RegisterReflection(reflectionDB);

		const Hush::Reflection::TypeInfo *typeInfo = reflectionDB.GetTypeInfo("AutogenTest");

		REQUIRE(typeInfo != nullptr);

		char buffer[sizeof(AutogenTest)];

		int value = 50;
		std::optional<Hush::Reflection::TypeInfo::EInPlaceConstructorError> result =
			typeInfo->CreateInPlaceInstance(buffer, sizeof(buffer), {Hush::Reflection::VariantView(&value)});

		REQUIRE(!result.has_value());

		// Okay, buffer now contains an instance of AutogenTest
		AutogenTest *instance = reinterpret_cast<AutogenTest *>(buffer);
		REQUIRE(instance->myCustomField == value);
	}

	SECTION("Types registered without a module belong to the engine")
	{
		Hush::Reflection::ReflectionDB reflectionDB;
		AutogenTest::RegisterReflection(reflectionDB);

		const Hush::Reflection::TypeInfo *typeInfo = reflectionDB.GetTypeInfo("AutogenTest");
		REQUIRE(typeInfo != nullptr);
		REQUIRE(typeInfo->GetOwner() == Hush::ENGINE_MODULE_HANDLE);
		REQUIRE(typeInfo->IsBuiltin());
	}

	SECTION("Duplicate registration returns an error and keeps the first type")
	{
		Hush::Reflection::ReflectionDB reflectionDB;
		AutogenTest::RegisterReflection(reflectionDB);

		Hush::Reflection::TypeInfo duplicate(Hush::Reflection::GetTypeId<AutogenTest>());
		duplicate.SetName("AutogenTest");
		duplicate.SetSize(1);
		REQUIRE(reflectionDB.RegisterClass(std::move(duplicate)) ==
				Hush::Reflection::ERegisterClassError::DuplicateType);

		const Hush::Reflection::TypeInfo *typeInfo = reflectionDB.GetTypeInfo("AutogenTest");
		REQUIRE(typeInfo != nullptr);
		REQUIRE(typeInfo->GetSize() == sizeof(AutogenTest));
	}

	SECTION("Module types can be unregistered")
	{
		Hush::Reflection::ReflectionDB reflectionDB;
		constexpr Hush::ModuleHandle module = 42;

		AutogenTest::RegisterReflection(reflectionDB, module);
		REQUIRE(reflectionDB.GetTypeInfo("AutogenTest") != nullptr);
		REQUIRE(reflectionDB.GetTypeInfo("AutogenTest")->GetOwner() == module);

		REQUIRE(reflectionDB.UnregisterModule(module) == 1);
		REQUIRE(reflectionDB.GetTypeInfo("AutogenTest") == nullptr);

		// The engine module cannot be unregistered.
		AutogenTest::RegisterReflection(reflectionDB);
		REQUIRE(reflectionDB.UnregisterModule(Hush::ENGINE_MODULE_HANDLE) == 0);
		REQUIRE(reflectionDB.GetTypeInfo("AutogenTest") != nullptr);
	}

	SECTION("Metadata is stored in the type info")
	{
		Hush::Reflection::ReflectionDB reflectionDB;

		reflectionDB.RegisterClass<AutogenTest>()
			.AddMetadata(Hush::Reflection::METADATA_KEY_BUILTIN.data(), "true")
			.AddMetadata("category", "test")
			.Register();

		const Hush::Reflection::TypeInfo *typeInfo = reflectionDB.GetTypeInfo("AutogenTest");
		REQUIRE(typeInfo != nullptr);
		REQUIRE(typeInfo->HasMetadata("category"));
		REQUIRE(typeInfo->GetMetadata("category").value() == "test");
		REQUIRE(typeInfo->HasMetadata(Hush::Reflection::METADATA_KEY_BUILTIN));
		REQUIRE(!typeInfo->HasMetadata("missing"));
	}
}
