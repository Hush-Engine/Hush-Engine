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
			.AddProperty(Hush::Reflection::FieldInfo(Hush::Reflection::GetTypeId<int>(), "a", aSetter, aGetter))
			.Register();
	}
};

TEST_CASE("Reflection", "[reflection]")
{
	SECTION("Call reflection")
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
}
