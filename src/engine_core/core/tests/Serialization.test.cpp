/*! \file Serialization.test.cpp
	\author Alan Ramirez
	\date 2025-02-10
	\brief Entity test implementation
*/
#include <serialization/Formats/JsonSerializer.hpp>
#include <serialization/Serialization.hpp>
#include <catch2/catch_test_macros.hpp>
#include <array>

struct Vector3
{
	float x{}, y{}, z{};

	Hush::Serialization::ESerializationError Serialize(Hush::Serialization::JsonSerializer &serializer) const
	{
		auto error = serializer.Serialize<std::string_view>("__type", "Vector3");
		if (error != Hush::Serialization::ESerializationError::None)
		{
			return error;
		}

		error = serializer.Serialize("x", x);
		if (error != Hush::Serialization::ESerializationError::None)
		{
			return error;
		}

		error = serializer.Serialize("y", y);
		if (error != Hush::Serialization::ESerializationError::None)
		{
			return error;
		}

		return serializer.Serialize("z", z);
	}

	auto Deserialize(Hush::Serialization::IVisitor *parent, Hush::Serialization::EFormatDescribingType format)
	{
		struct Visitor : public Hush::Serialization::IVisitor
		{
			Hush::Serialization::BuiltinVisitors::Visitor<float> xVisitor;
			Hush::Serialization::BuiltinVisitors::Visitor<float> yVisitor;
			Hush::Serialization::BuiltinVisitors::Visitor<float> zVisitor;

			enum class EVisitorStatus
			{
				None,
				X,
				Y,
				Z,
			};

			EVisitorStatus status = EVisitorStatus::None;
			bool insideObject{false};

			explicit Visitor(IVisitor *parent, Vector3 &vec, Hush::Serialization::EFormatDescribingType format)
				: IVisitor(parent, format),
				  xVisitor(this, &vec.x, format),
				  yVisitor(this, &vec.y, format),
				  zVisitor(this, &vec.z, format)
			{
				if (format == Hush::Serialization::EFormatDescribingType::NonSelfDescribing)
				{
					SetStartingVisitor(&xVisitor);
					xVisitor.SetParentVisitor(&yVisitor);
					yVisitor.SetParentVisitor(&zVisitor);
					zVisitor.SetParentVisitor(GetParentVisitor());
				}
				else
				{
					SetStartingVisitor(this);
				}
			}

			Result VisitObjectStart() override
			{
				if (insideObject)
				{
					return Hush::Serialization::EDeserializationError::InvalidData;
				}

				insideObject = true;

				return this;
			}

			Result VisitObjectEnd() override
			{
				if (!insideObject)
				{
					return Hush::Serialization::EDeserializationError::InvalidData;
				}

				return nullptr;
			}

			Result VisitKey(std::string_view value) override
			{
				if (!insideObject)
				{
					return Hush::Serialization::EDeserializationError::InvalidData;
				}

				if (value == "x")
				{
					status = EVisitorStatus::X;
					return &xVisitor;
				}
				if (value == "y")
				{
					status = EVisitorStatus::Y;
					return &yVisitor;
				}
				if (value == "z")
				{
					status = EVisitorStatus::Z;
					return &zVisitor;
				}

				return Hush::Serialization::EDeserializationError::InvalidKey;
			}
		};

		return Visitor{parent, *this, format};
	}
};

struct SerializableStruct
{
	int a{0};
	float b{1};
	Vector3 c;
	std::array<std::string, 3> d{};

	Hush::Serialization::ESerializationError Serialize(Hush::Serialization::JsonSerializer &serializer) const
	{
		auto error = serializer.Serialize<std::string_view>("__type", "SerializableStruct");
		if (error != Hush::Serialization::ESerializationError::None)
		{
			return error;
		}
		error = serializer.Serialize("a", a);
		if (error != Hush::Serialization::ESerializationError::None)
		{
			return error;
		}

		error = serializer.Serialize("b", b);

		if (error != Hush::Serialization::ESerializationError::None)
		{
			return error;
		}

		error = serializer.Serialize("c", c);
		if (error != Hush::Serialization::ESerializationError::None)
		{
			return error;
		}

		return serializer.Serialize<std::string>("d", d);
	}
};

TEST_CASE("Serialization", "[serialization]")
{
	SECTION("Serialize reflection")
	{
		Hush::Serialization::JsonSerializer jsonSerializer;

		constexpr std::string_view EXPECTED_JSON =
			R"({"__type":"SerializableStruct","a":10,"b":20.0,"c":{"__type":"Vector3","x":0.0,"y":0.0,"z":0.0},"d":["","",""]})";

		SerializableStruct serializableStruct;
		serializableStruct.a = 10;
		serializableStruct.b = 20.0f;

		auto result = Hush::Serialization::SerializeJson(serializableStruct);

		REQUIRE(result.has_value());

		std::string json = result.value();
		REQUIRE(json == EXPECTED_JSON);
	}
}

TEST_CASE("Deserialization", "[serialization]")
{
	SECTION("Deserialize")
	{
		constexpr std::string_view json = R"( {
			"x": 10.0,
			"y": 20.0,
			"z": 30.0
		})";

		Hush::Result<Vector3, Hush::Serialization::EDeserializationError> result =
			Hush::Serialization::DeserializeJson<Vector3>(json);

		REQUIRE(result.has_value());

		Vector3 vec = result.value();

		REQUIRE(vec.x == 10);
		REQUIRE(vec.y == 20.0f);
		REQUIRE(vec.z == 30.0f);
	}
}