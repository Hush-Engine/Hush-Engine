/*! \file Serialization.test.cpp
	\author Alan Ramirez
	\date 2025-02-10
	\brief Entity test implementation
*/
#include <serialization/Formats/JsonSerializer.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "serialization/GeneratedSerialization.hpp"
#include "serialization/GeneratedSerialization2.hpp"

struct Vector3
{
	float x{}, y{}, z{};

	template <typename T>
	Hush::Serialization::ESerializationError Serialize(T &serializer) const
	{
		auto error = serializer.template Serialize<std::string_view>("__type", "Vector3");
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
			Hush::Serialization::Visitor<float> xVisitor;
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

			explicit Visitor(IVisitor *parent, Vector3 *vec, Hush::Serialization::EFormatDescribingType format)
				: IVisitor(parent, format),
				  xVisitor(this, &vec->x, format),
				  yVisitor(this, &vec->y, format),
				  zVisitor(this, &vec->z, format)
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

				return GetParentVisitor();
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

		return Visitor{parent, this, format};
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

		return serializer.Serialize("d", std::span<const std::string>(d));
	}
};

struct SerializableDemo
{
	Vector3 c;

	auto Deserialize(Hush::Serialization::IVisitor *parent, Hush::Serialization::EFormatDescribingType format)
	{
		struct Visitor : public Hush::Serialization::IVisitor
		{
			Hush::Serialization::Visitor<Vector3> cVisitor;
			bool insideObject{false};

			explicit Visitor(IVisitor *parent, SerializableDemo &demo,
							 Hush::Serialization::EFormatDescribingType format)
				: IVisitor(parent, format),
				  cVisitor(this, &demo.c, format)
			{
				if (format == Hush::Serialization::EFormatDescribingType::NonSelfDescribing)
				{
					SetStartingVisitor(&cVisitor);
					cVisitor.SetParentVisitor(GetParentVisitor());
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

				insideObject = false;

				return GetParentVisitor();
			}

			Result VisitKey(std::string_view value) override
			{
				if (!insideObject)
				{
					return Hush::Serialization::EDeserializationError::InvalidData;
				}

				if (value == "c")
				{
					return &cVisitor;
				}

				return Hush::Serialization::EDeserializationError::InvalidKey;
			}
		};

		return Visitor{parent, *this, format};
	}
};

struct Mat4Serializable
{
	glm::mat4 matrix{1.0f};

	template <typename T>
	Hush::Serialization::ESerializationError Serialize(T &serializer) const
	{
		auto error = serializer.template Serialize<std::string_view>("__type", "Mat4Serializable");
		if (error != Hush::Serialization::ESerializationError::None)
		{
			return error;
		}

		return serializer.Serialize("matrix", matrix);
	}

	auto Deserialize(Hush::Serialization::IVisitor *parent, Hush::Serialization::EFormatDescribingType format)
	{
		struct Visitor : public Hush::Serialization::IVisitor
		{
			Hush::Serialization::Visitor<glm::mat4> matrixVisitor;
			bool insideObject{false};

			explicit Visitor(IVisitor *parent, Mat4Serializable *instance,
							 Hush::Serialization::EFormatDescribingType format)
				: IVisitor(parent, format),
				  matrixVisitor(this, &instance->matrix, format)
			{
				SetStartingVisitor(this);
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

				return GetParentVisitor();
			}

			Result VisitKey(std::string_view value) override
			{
				if (!insideObject)
				{
					return Hush::Serialization::EDeserializationError::InvalidData;
				}

				if (value == "matrix")
				{
					return &matrixVisitor;
				}

				return Hush::Serialization::EDeserializationError::InvalidKey;
			}
		};

		return Visitor{parent, this, format};
	}
};

struct Vec4Serializable
{
	glm::vec4 vector{1.0f};

	template <typename T>
	Hush::Serialization::ESerializationError Serialize(T &serializer) const
	{
		auto error = serializer.template Serialize<std::string_view>("__type", "Vec4Serializable");
		if (error != Hush::Serialization::ESerializationError::None)
		{
			return error;
		}

		return serializer.Serialize("vector", vector);
	}

	auto Deserialize(Hush::Serialization::IVisitor *parent, Hush::Serialization::EFormatDescribingType format)
	{
		struct Visitor : public Hush::Serialization::IVisitor
		{
			Hush::Serialization::Visitor<glm::vec4> vectorVisitor;
			bool insideObject{false};

			explicit Visitor(IVisitor *parent, Vec4Serializable *instance,
							 Hush::Serialization::EFormatDescribingType format)
				: IVisitor(parent, format),
				  vectorVisitor(this, &instance->vector, format)
			{
				SetStartingVisitor(this);
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

				return GetParentVisitor();
			}

			Result VisitKey(std::string_view value) override
			{
				if (!insideObject)
				{
					return Hush::Serialization::EDeserializationError::InvalidData;
				}

				if (value == "vector")
				{
					return &vectorVisitor;
				}

				return Hush::Serialization::EDeserializationError::InvalidKey;
			}
		};

		return Visitor{parent, this, format};
	}
};

struct TolerantNested
{
	glm::vec4 m_rgba{1.0f};

	auto Deserialize(Hush::Serialization::IVisitor *parent, Hush::Serialization::EFormatDescribingType format)
	{
		struct Visitor : public Hush::Serialization::IVisitor
		{
			Hush::Serialization::Visitor<glm::vec4> rgbaVisitor;
			bool insideObject{false};

			explicit Visitor(IVisitor *parent, TolerantNested *instance,
							 Hush::Serialization::EFormatDescribingType format)
				: IVisitor(parent, format),
				  rgbaVisitor(this, &instance->m_rgba, format)
			{
				SetStartingVisitor(this);
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

				return GetParentVisitor();
			}

			Result VisitKey(std::string_view value) override
			{
				if (!insideObject)
				{
					return Hush::Serialization::EDeserializationError::InvalidData;
				}

				if (value == "m_rgba")
				{
					return &rgbaVisitor;
				}

				// Unknown member; the bridge skips its value.
				return this;
			}
		};

		return Visitor{parent, this, format};
	}
};

struct TolerantLight
{
	float intensity{1.0f};
	TolerantNested color{};

	auto Deserialize(Hush::Serialization::IVisitor *parent, Hush::Serialization::EFormatDescribingType format)
	{
		struct Visitor : public Hush::Serialization::IVisitor
		{
			Hush::Serialization::Visitor<float> intensityVisitor;
			Hush::Serialization::Visitor<TolerantNested> colorVisitor;
			bool insideObject{false};

			explicit Visitor(IVisitor *parent, TolerantLight *instance,
							 Hush::Serialization::EFormatDescribingType format)
				: IVisitor(parent, format),
				  intensityVisitor(this, &instance->intensity, format),
				  colorVisitor(this, &instance->color, format)
			{
				SetStartingVisitor(this);
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

				return GetParentVisitor();
			}

			Result VisitKey(std::string_view value) override
			{
				if (!insideObject)
				{
					return Hush::Serialization::EDeserializationError::InvalidData;
				}

				if (value == "intensity")
				{
					return &intensityVisitor;
				}

				if (value == "color")
				{
					return &colorVisitor;
				}

				// Unknown member; the bridge skips its value.
				return this;
			}
		};

		return Visitor{parent, this, format};
	}
};

TEST_CASE("Serialization", "[serialization]")
{
	SECTION("Serialize reflection")
	{
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

	SECTION("Autogen serialization")
	{
		constexpr std::string_view EXPECTED_JSON =
			R"({"__type":"SerializationAutogenStruct2","field":{"__type":"SerializationAutogenStruct","m_value1":5,"m_value2":15}})";
		SerializationAutogenStruct2 serializationStruct;

		serializationStruct.field.SetValue1(5);
		serializationStruct.field.SetValue2(15);

		auto result = Hush::Serialization::SerializeJson(serializationStruct);
		REQUIRE(result.has_value());

		std::string json = result.value();

		REQUIRE(json == EXPECTED_JSON);
	}

	SECTION("Serialize glm::mat4")
	{
		constexpr std::string_view EXPECTED_JSON =
			R"([1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0])";

		const glm::mat4 matrix(1.0f);

		auto result = Hush::Serialization::SerializeJson(matrix);

		REQUIRE(result.has_value());

		std::string json = result.value();

		REQUIRE(json == EXPECTED_JSON);
	}

	SECTION("Serialize glm::vec4")
	{
		constexpr std::string_view EXPECTED_JSON = R"([1.0,2.0,3.0,4.0])";

		const glm::vec4 vector(1.0f, 2.0f, 3.0f, 4.0f);

		auto result = Hush::Serialization::SerializeJson(vector);

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

		static_assert(std::is_same_v<std::true_type, Hush::Serialization::BuiltinVisitors::Visitor<float>::Exists>,
					  "Visitor for float should exist");

		REQUIRE(result.has_value());

		Vector3 vec = result.value();

		REQUIRE(vec.x == 10);
		REQUIRE(vec.y == 20.0f);
		REQUIRE(vec.z == 30.0f);
	}

	SECTION("Nested object")
	{
		constexpr std::string_view json = R"( {
			"c": {
				"x": 10.0,
				"y": 20.0,
				"z": 30.0
			}
		})";

		Hush::Result<SerializableDemo, Hush::Serialization::EDeserializationError> result =
			Hush::Serialization::DeserializeJson<SerializableDemo>(json);

		REQUIRE(result.has_value());

		SerializableDemo demo = result.value();

		REQUIRE(demo.c.x == 10);
		REQUIRE(demo.c.y == 20.0f);
		REQUIRE(demo.c.z == 30.0f);
	}

	SECTION("Autogen deserialization")
	{
		constexpr std::string_view json = R"( {
			"m_value1": 1,
			"m_value2": 2
		})";

		Hush::Result<SerializationAutogenStruct, Hush::Serialization::EDeserializationError> result =
			Hush::Serialization::DeserializeJson<SerializationAutogenStruct>(json);

		REQUIRE(result.has_value());
		SerializationAutogenStruct serializationStruct = result.value();
		REQUIRE(serializationStruct.GetValue1() == 1);
		REQUIRE(serializationStruct.GetValue2() == 2);
	}

	SECTION("Deserialize glm::mat4 member")
	{
		constexpr std::string_view json = R"( {
			"matrix": [1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0]
		})";

		Hush::Result<Mat4Serializable, Hush::Serialization::EDeserializationError> result =
			Hush::Serialization::DeserializeJson<Mat4Serializable>(json);

		REQUIRE(result.has_value());

		Mat4Serializable value = result.value();
		const float *got = glm::value_ptr(value.matrix);
		const glm::mat4 identity(1.0f);
		const float *want = glm::value_ptr(identity);

		for (int i = 0; i < 16; ++i)
		{
			REQUIRE(got[i] == want[i]);
		}
	}

	SECTION("Deserialize glm::vec4 member")
	{
		constexpr std::string_view json = R"( {
			"vector": [1.0,2.0,3.0,4.0]
		})";

		Hush::Result<Vec4Serializable, Hush::Serialization::EDeserializationError> result =
			Hush::Serialization::DeserializeJson<Vec4Serializable>(json);

		REQUIRE(result.has_value());

		Vec4Serializable value = result.value();
		const float *got = glm::value_ptr(value.vector);
		const glm::vec4 expected(1.0f, 2.0f, 3.0f, 4.0f);
		const float *want = glm::value_ptr(expected);

		for (int i = 0; i < 4; ++i)
		{
			REQUIRE(got[i] == want[i]);
		}
	}

	SECTION("Raw walker")
	{
		constexpr std::string_view json = R"({
			"name": "test",
			"count": 42,
			"ratio": 1.5,
			"enabled": true,
			"items": [1, 2, 3]
		})";

		Hush::Serialization::JsonDeserializer deserializer(json);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		std::string_view key;
		std::string_view str;
		double num = 0.0;
		bool b = false;

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "name");
		REQUIRE(deserializer.ReadString(str));
		REQUIRE(str == "test");

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "count");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 42.0);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "ratio");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 1.5);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "enabled");
		REQUIRE(deserializer.ReadBool(b));
		REQUIRE(b);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "items");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ArrayStart);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::Uint);
		REQUIRE(deserializer.GetDouble() == 1.0);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetDouble() == 2.0);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetDouble() == 3.0);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ArrayEnd);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);

		REQUIRE(!deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::EndOfInput);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker skip value")
	{
		constexpr std::string_view json = R"({
			"keep": 1,
			"nested": { "a": [1, 2], "b": "x" },
			"after": true
		})";

		Hush::Serialization::JsonDeserializer deserializer(json);

		std::string_view key;
		bool b = false;

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "keep");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::Uint);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "nested");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);
		REQUIRE(deserializer.SkipValue());

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "after");
		REQUIRE(deserializer.ReadBool(b));
		REQUIRE(b);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);
		REQUIRE(!deserializer.Next());
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker peek key")
	{
		constexpr std::string_view json = R"({ "a": 1, "b": 2 })";

		Hush::Serialization::JsonDeserializer deserializer(json);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		auto key = deserializer.PeekKey();
		REQUIRE(key.has_value());
		REQUIRE(*key == "a");

		// Peeking again returns the same key without consuming it.
		auto keyAgain = deserializer.PeekKey();
		REQUIRE(keyAgain.has_value());
		REQUIRE(*keyAgain == "a");

		// Next() now consumes the peeked key.
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::Key);
		REQUIRE(deserializer.GetKey() == "a");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::Uint);
		REQUIRE(deserializer.GetDouble() == 1.0);

		auto keyB = deserializer.PeekKey();
		REQUIRE(keyB.has_value());
		REQUIRE(*keyB == "b");

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::Key);
		REQUIRE(deserializer.GetKey() == "b");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::Uint);
		REQUIRE(deserializer.GetDouble() == 2.0);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);
		REQUIRE(!deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::EndOfInput);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker peek object")
	{
		constexpr std::string_view json = R"({ "a": 1, "nested": { "x": [1, 2], "y": "z" }, "b": 2 })";

		Hush::Serialization::JsonDeserializer deserializer(json);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		std::string_view key;
		double num = 0.0;

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "a");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 1.0);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "nested");

		std::string_view obj;
		REQUIRE(deserializer.PeekObject(obj));
		REQUIRE(obj == R"({ "x": [1, 2], "y": "z" })");

		// PeekObject is non-destructive: peeking again returns the same object.
		std::string_view obj2;
		REQUIRE(deserializer.PeekObject(obj2));
		REQUIRE(obj2 == obj);

		// The object-start token is still buffered; Next() consumes it.
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		// The object's contents can be walked afterwards.
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "x");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ArrayStart);
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::Uint);
		REQUIRE(deserializer.GetDouble() == 1.0);
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetDouble() == 2.0);
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ArrayEnd);
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "y");
		std::string_view str;
		REQUIRE(deserializer.ReadString(str));
		REQUIRE(str == "z");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);

		// Peeking a non-object token returns false and leaves it buffered.
		std::string_view notObj;
		REQUIRE(!deserializer.PeekObject(notObj));

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "b");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 2.0);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);
		REQUIRE(!deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::EndOfInput);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker skip object")
	{
		constexpr std::string_view json = R"({ "a": 1, "nested": { "x": [1, 2] }, "b": 2 })";

		Hush::Serialization::JsonDeserializer deserializer(json);

		// Skipping before any token is read does nothing and does not advance.
		REQUIRE(!deserializer.SkipObject());

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		std::string_view key;
		double num = 0.0;
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "a");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 1.0);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "nested");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);
		REQUIRE(deserializer.SkipObject());

		// The walker is now past the nested object, on the next key.
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "b");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 2.0);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);
		REQUIRE(!deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::EndOfInput);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker skip object after peek")
	{
		constexpr std::string_view json = R"({ "nested": { "x": [1] }, "after": true })";

		Hush::Serialization::JsonDeserializer deserializer(json);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		std::string_view key;
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "nested");

		std::string_view obj;
		REQUIRE(deserializer.PeekObject(obj));
		REQUIRE(obj == R"({ "x": [1] })");

		// The buffered object start can be skipped.
		REQUIRE(deserializer.SkipObject());

		bool b = false;
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "after");
		REQUIRE(deserializer.ReadBool(b));
		REQUIRE(b);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);
		REQUIRE(!deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::EndOfInput);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker skip object mid-scope")
	{
		constexpr std::string_view json = R"({ "a": 1, "keep": { "x": 1 }, "b": 2 })";

		Hush::Serialization::JsonDeserializer deserializer(json);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		std::string_view key;
		double num = 0.0;
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "a");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 1.0);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "keep");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "x");

		// In the middle of the "keep" object; skip the rest of it.
		REQUIRE(deserializer.SkipObject());

		// Back in the root object, on the next key.
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "b");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 2.0);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);
		REQUIRE(!deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::EndOfInput);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker skip object mid-scope with nesting")
	{
		// Skipping from deep inside nested containers jumps to the innermost
		// enclosing object scope.
		constexpr std::string_view json = R"({ "outer": { "x": 1, "inner": { "y": 2 }, "z": 3 }, "tail": 4 })";

		Hush::Serialization::JsonDeserializer deserializer(json);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		std::string_view key;
		double num = 0.0;
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "outer");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "x");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 1.0);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "inner");
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "y");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 2.0);

		// Skipping from the middle of "inner" jumps to after its closing brace,
		// i.e. back inside "outer".
		REQUIRE(deserializer.SkipObject());

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "z");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 3.0);

		// "outer" closes, then we're back at the root object.
		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);

		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "tail");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 4.0);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);
		REQUIRE(!deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::EndOfInput);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker skip root object from middle")
	{
		constexpr std::string_view json = R"({ "a": 1, "b": { "c": 2 } })";

		Hush::Serialization::JsonDeserializer deserializer(json);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		std::string_view key;
		double num = 0.0;
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "a");
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 1.0);

		// Skipping the root object from its middle consumes the whole document.
		REQUIRE(deserializer.SkipObject());

		REQUIRE(!deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::EndOfInput);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker read object")
	{
		constexpr std::string_view json = R"({ "nested": { "x": [1], "y": 2 }, "after": true })";

		Hush::Serialization::JsonDeserializer deserializer(json);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		std::string_view key;
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "nested");

		std::string_view obj;
		REQUIRE(deserializer.ReadObject(obj));
		REQUIRE(obj == R"({ "x": [1], "y": 2 })");

		// The walker advanced past the object, back inside the root.
		bool b = false;
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "after");
		REQUIRE(deserializer.ReadBool(b));
		REQUIRE(b);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectEnd);
		REQUIRE(!deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::EndOfInput);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Raw walker read object non-object")
	{
		constexpr std::string_view json = R"({ "value": 5 })";

		Hush::Serialization::JsonDeserializer deserializer(json);

		REQUIRE(deserializer.Next());
		REQUIRE(deserializer.GetToken() == Hush::Serialization::JsonDeserializer::EToken::ObjectStart);

		std::string_view key;
		REQUIRE(deserializer.ReadKey(key));
		REQUIRE(key == "value");

		std::string_view obj;
		REQUIRE(!deserializer.ReadObject(obj));

		// The non-object token is still consumable.
		double num = 0.0;
		REQUIRE(deserializer.ReadDouble(num));
		REQUIRE(num == 5.0);
		REQUIRE(!deserializer.HasError());
	}

	SECTION("Deserialize ignores unknown members")
	{
		// Mirrors the serialized component shape: metadata fields ("id", "key",
		// "__type") that are not part of the type definition, plus a nested
		// reflected member that carries its own "__type".
		constexpr std::string_view json = R"({
			"id": 569,
			"key": "DirectionalLight",
			"__type": "Hush::DirectionalLight",
			"intensity": 2.5,
			"color": { "__type": "Hush::Color", "m_rgba": [0.1, 0.2, 0.3, 0.4] }
		})";

		TolerantLight instance;
		Hush::Serialization::JsonDeserializer deserializer(json);
		Hush::Serialization::EDeserializationError err = deserializer.Deserialize<TolerantLight>(&instance);

		REQUIRE(err == Hush::Serialization::EDeserializationError::None);
		REQUIRE(instance.intensity == 2.5f);

		const float *rgba = glm::value_ptr(instance.color.m_rgba);
		REQUIRE(rgba[0] == 0.1f);
		REQUIRE(rgba[1] == 0.2f);
		REQUIRE(rgba[2] == 0.3f);
		REQUIRE(rgba[3] == 0.4f);
	}

	SECTION("Deserialize ignores unknown members with container values")
	{
		constexpr std::string_view json = R"({
			"unknownObj": { "a": [1, 2], "b": { "c": "x" } },
			"unknownArr": [ { "d": 1 }, 2 ],
			"intensity": 3.0
		})";

		TolerantLight instance;
		Hush::Serialization::JsonDeserializer deserializer(json);
		Hush::Serialization::EDeserializationError err = deserializer.Deserialize<TolerantLight>(&instance);

		REQUIRE(err == Hush::Serialization::EDeserializationError::None);
		REQUIRE(instance.intensity == 3.0f);
	}
}
