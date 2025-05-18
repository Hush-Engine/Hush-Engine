/*! \file JsonSerializer.cpp
	\author Alan Ramirez
	\date 2025-05-17
	\brief Serialization/deserialization for json
*/

#include "JsonSerializer.hpp"

#include "Assertions.hpp"

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Null()
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");
	auto result = visitor->VisitNull();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Bool(bool b)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	auto result = visitor->VisitBool(b);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Int(int i)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	auto result = visitor->VisitInt32(i);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Uint(unsigned i)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	auto result = visitor->VisitUInt32(i);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Int64(int64_t i)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	auto result = visitor->VisitInt64(i);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Uint64(uint64_t i)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	auto result = visitor->VisitUInt64(i);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Double(double d)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	auto result = visitor->VisitDouble(d);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::RawNumber(const Ch *str, rapidjson::SizeType length,
																		bool copy)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	(void)copy;

	auto result = visitor->VisitString(std::string_view(str, length));
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::String(const Ch *str, rapidjson::SizeType length,
																	 bool copy)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	(void)copy;

	auto result = visitor->VisitString(std::string_view(str, length));
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::StartObject()
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	auto result = visitor->VisitObjectStart();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Key(const Ch *str, rapidjson::SizeType length, bool copy)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	(void)copy;

	auto result = visitor->VisitKey(std::string_view(str, length));
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::EndObject(rapidjson::SizeType memberCount)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	(void)memberCount;

	auto result = visitor->VisitObjectEnd();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::StartArray()
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	auto result = visitor->VisitArrayStart();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::EndArray(rapidjson::SizeType elementCount)
{
	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	(void)elementCount;

	auto result = visitor->VisitArrayEnd();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}