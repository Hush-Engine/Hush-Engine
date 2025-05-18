/*! \file JsonSerializer.hpp
	\author Alan Ramirez
	\date 2025-05-17
	\brief Serialization/deserialization for json
*/

#include "JsonSerializer.hpp"

/**
 * @brief Handles a JSON null value during deserialization.
 *
 * Delegates processing of a JSON null to the underlying visitor. Returns false if an error occurs; otherwise, updates the visitor state and returns true.
 *
 * @return true if the null value was processed successfully; false if an error occurred.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Null()
{
	auto result = visitor->VisitNull();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles a JSON boolean value during deserialization.
 *
 * Delegates the boolean value to the underlying visitor. Returns false if the visitor reports an error; otherwise, updates the visitor state and returns true.
 *
 * @param b The boolean value encountered in the JSON input.
 * @return true if the value was processed successfully; false if an error occurred.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Bool(bool b)
{
	auto result = visitor->VisitBool(b);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles a JSON integer value during deserialization.
 *
 * Delegates the integer value to the underlying visitor's VisitInt32 method. Returns false if an error occurs; otherwise, updates the visitor state and returns true.
 *
 * @param i The integer value encountered in the JSON input.
 * @return true if the value was processed successfully; false if an error occurred.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Int(int i)
{
	auto result = visitor->VisitInt32(i);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles an unsigned integer value during JSON deserialization.
 *
 * Delegates the unsigned integer event to the underlying visitor. Returns false if the visitor reports an error; otherwise, updates the visitor state and returns true.
 *
 * @param i The unsigned integer value encountered in the JSON input.
 * @return true if the value was processed successfully; false if an error occurred.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Uint(unsigned i)
{
	auto result = visitor->VisitUInt32(i);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles a 64-bit integer value during JSON deserialization.
 *
 * Passes the 64-bit integer to the underlying visitor and updates the visitor state. Returns false if an error occurs.
 *
 * @param i The 64-bit integer value encountered in the JSON input.
 * @return true if the value was processed successfully; false if an error was detected.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Int64(int64_t i)
{
	auto result = visitor->VisitInt64(i);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles a JSON unsigned 64-bit integer value during deserialization.
 *
 * Delegates processing of the unsigned 64-bit integer to the underlying visitor. Returns false if an error occurs; otherwise, updates the visitor state and returns true.
 *
 * @param i The unsigned 64-bit integer value encountered in the JSON input.
 * @return true if the value was processed successfully; false if an error occurred.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Uint64(uint64_t i)
{
	auto result = visitor->VisitUInt64(i);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles a JSON double value during deserialization.
 *
 * Delegates processing of a floating-point value to the underlying visitor. Returns false if an error occurs; otherwise, updates the visitor state and returns true.
 *
 * @param d The double value encountered in the JSON input.
 * @return true if the value was processed successfully; false if an error occurred.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Double(double d)
{
	auto result = visitor->VisitDouble(d);
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles a raw number token from RapidJSON by forwarding it as a string to the visitor.
 *
 * Converts the raw number to a string view and delegates processing to the underlying visitor's VisitString method.
 *
 * @param str Pointer to the character data representing the raw number.
 * @param length Length of the character data.
 * @return true if the visitor processed the string without error; false otherwise.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::RawNumber(const Ch *str, rapidjson::SizeType length,
																		bool copy)
{
	(void)copy;

	auto result = visitor->VisitString(std::string_view(str, length));
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles the start of a JSON object during deserialization.
 *
 * Delegates the event to the underlying visitor and updates the visitor state. Returns false if an error occurs.
 * @return true if the visitor successfully processes the object start; false if an error is encountered.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::StartObject()
{
	auto result = visitor->VisitObjectStart();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles a JSON object key event during deserialization.
 *
 * Passes the key string to the underlying visitor for processing. Returns false if the visitor reports an error; otherwise, updates the visitor state and returns true.
 *
 * @param str Pointer to the key string.
 * @param length Length of the key string.
 * @return true if the key was processed successfully; false if an error occurred.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Key(const Ch *str, rapidjson::SizeType length, bool copy)
{
	(void)copy;

	auto result = visitor->VisitKey(std::string_view(str, length));
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles the end of a JSON object during deserialization.
 *
 * Signals the visitor that a JSON object has ended. Returns false if the visitor reports an error; otherwise, updates the visitor state and returns true.
 *
 * @param memberCount The number of members in the object (unused).
 * @return true if the operation succeeds; false if an error occurs.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::EndObject(rapidjson::SizeType memberCount)
{
	(void)memberCount;

	auto result = visitor->VisitObjectEnd();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles the start of a JSON array during deserialization.
 *
 * Delegates the array start event to the underlying visitor and updates the visitor state. Returns false if an error occurs.
 * @return true if the visitor successfully processes the array start; false if an error is encountered.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::StartArray()
{
	auto result = visitor->VisitArrayStart();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}

/**
 * @brief Handles the end of a JSON array during deserialization.
 *
 * Calls the underlying visitor's VisitArrayEnd method and updates the visitor state. Returns false if an error occurs.
 * 
 * @param elementCount The number of elements in the array (unused).
 * @return true if the operation succeeds; false if an error is encountered.
 */
bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::EndArray(rapidjson::SizeType elementCount)
{
	(void)elementCount;

	auto result = visitor->VisitArrayEnd();
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();
	return true;
}