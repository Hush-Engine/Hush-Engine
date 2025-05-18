/*! \file JsonSerializer.hpp
	\author Alan Ramirez
	\date 2025-05-17
	\brief Serialization/deserialization for json
*/

#pragma once

#include <Result.hpp>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <string_view>
#include <span>
#include "../Serialization.hpp"
#include "../Deserialization.hpp"

namespace Hush::Serialization
{
	///
	/// JsonSerializer class.
	///
	/// This class is used to serialize data to a JSON string.
	/// It has member functions to set keys, start and end objects, and serialize data.
	/// It also supports serializing custom types that follow the @ref IsSerializable concept.
	class JsonSerializer
	{
	public:
		/**
		 * @brief Constructs a JsonSerializer with an empty buffer and writer.
		 *
		 * Initializes the internal RapidJSON buffer and writer for subsequent JSON serialization operations.
		 */
		JsonSerializer()
			: m_writer(m_buffer)
		{
		}

		/// Sets the key for the next value to be serialized.
		/// @param key Key to serialize
		/**
		 * @brief Sets the key for the next JSON value in the serialization process.
		 *
		 * @param key The key to use for the next JSON value.
		 * @return ESerializationError Returns None on success, or InvalidData if the key is invalid or cannot be set.
		 */
		[[nodiscard]]
		ESerializationError SetKey(std::string_view key)
		{
			return m_writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.size()))
					   ? ESerializationError::None
					   : ESerializationError::InvalidData;
		}

		/// Begins an object in the JSON string.
		/**
		 * @brief Begins a new JSON object in the serialization stream.
		 *
		 * @return ESerializationError Returns None on success, or InvalidData if the object could not be started.
		 */
		[[nodiscard]]
		ESerializationError BeginObject()
		{
			return m_writer.StartObject() ? ESerializationError::None : ESerializationError::InvalidData;
		}

		/// Ends an object in the JSON string.
		/**
		 * @brief Ends the current JSON object in the serialization stream.
		 *
		 * @return ESerializationError Returns None on success, or InvalidData if ending the object fails.
		 */
		[[nodiscard]]
		ESerializationError EndObject()
		{
			return m_writer.EndObject() ? ESerializationError::None : ESerializationError::InvalidData;
		}

		/// Serializes an object to a JSON string.
		///
		/// @tparam T Serializable object
		/// @param value Value to serialize
		/// @return SerializationError
		template <IsSerializable<JsonSerializer> T>
		/**
		 * @brief Serializes a custom object as a JSON object.
		 *
		 * Calls the object's `Serialize` method, writing its contents between JSON object delimiters.
		 *
		 * @tparam T Type that implements a `Serialize(JsonSerializer&)` method.
		 * @param value The object to serialize.
		 * @return ESerializationError Error code indicating success or failure.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const T &value)
		{
			m_writer.StartObject();
			ESerializationError error = value.Serialize(*this);
			m_writer.EndObject();

			return error;
		}

		template <typename T>
			requires(sizeof(T) > 16 && !IsSerializable<T, JsonSerializer>)
		/**
		 * @brief Fallback serialization method for unsupported types.
		 *
		 * This method triggers a compile-time error if called with a type that does not satisfy the serialization constraints.
		 *
		 * @return ESerializationError::InvalidType Always returned to indicate the type is not serializable.
		 */
		ESerializationError Serialize(const T &)
		{
			static_assert(false, "Type is not serializable");

			return ESerializationError::InvalidType;
		}

		template <typename T>
			requires(sizeof(T) <= 16 && !IsSerializable<T, JsonSerializer>)
		/**
		 * @brief Fallback serialization method for unsupported types.
		 *
		 * This method triggers a compile-time error if instantiated, indicating that the type is not serializable.
		 *
		 * @return ESerializationError Always returns ESerializationError::InvalidType.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const T)
		{
			static_assert(false, "Type is not serializable");

			return ESerializationError::InvalidType;
		}

		/// Serializes a double value to a JSON string.
		///
		/// @param key Key to serialize
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a double-precision floating-point value to JSON.
		 *
		 * @param value The double value to serialize.
		 * @return ESerializationError None on success, or InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const double value)
		{
			return m_writer.Double(value) ? ESerializationError::None : ESerializationError::InvalidData;
		}

		/// Serializes a float value to a JSON string.
		///
		/// @param key Key to serialize
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a float value as a JSON number.
		 *
		 * @param value The float to serialize.
		 * @return ESerializationError None on success, or an error code on failure.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const float value)
		{
			return Serialize<double>(value);
		}

		/// Serializes a boolean value to a JSON string.
		///
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a boolean value into the JSON output.
		 *
		 * @param value The boolean value to serialize.
		 * @return ESerializationError None on success, InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const bool value)
		{
			return m_writer.Bool(value) ? ESerializationError::None : ESerializationError::InvalidData;
		}

		/// Serializes an uint8 value to a JSON string.
		///
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes an 8-bit unsigned integer as a JSON number.
		 *
		 * @param value The uint8_t value to serialize.
		 * @return ESerializationError None on success, or InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::uint8_t value)
		{
			return !m_writer.Uint(value) ? ESerializationError::InvalidData : ESerializationError::None;
		}

		/// Serializes an uint16 value to a JSON string.
		///
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a 16-bit unsigned integer as a JSON number.
		 *
		 * @param value The uint16_t value to serialize.
		 * @return ESerializationError None on success, InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::uint16_t value)
		{
			return !m_writer.Uint(value) ? ESerializationError::InvalidData : ESerializationError::None;
		}

		/// Serializes an uint32 value to a JSON string.
		///
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a 32-bit unsigned integer as a JSON number.
		 *
		 * @param value The unsigned integer to serialize.
		 * @return ESerializationError Returns InvalidData if serialization fails; otherwise, None.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::uint32_t value)
		{
			return !m_writer.Uint(value) ? ESerializationError::InvalidData : ESerializationError::None;
		}

		/// Serializes an uint64 value to a JSON string.
		///
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a 64-bit unsigned integer as a JSON value.
		 *
		 * @param value The unsigned 64-bit integer to serialize.
		 * @return ESerializationError None on success, or InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::uint64_t value)
		{
			return !m_writer.Uint64(value) ? ESerializationError::InvalidData : ESerializationError::None;
		}

		/// Serializes an int8 value to a JSON string.
		///
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes an 8-bit signed integer as a JSON number.
		 *
		 * @param value The 8-bit signed integer to serialize.
		 * @return ESerializationError None on success, or InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::int8_t value)
		{
			return !m_writer.Int(value) ? ESerializationError::InvalidData : ESerializationError::None;
		}

		/// Serializes an int16 value to a JSON string.
		///
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a 16-bit signed integer value into the JSON output.
		 *
		 * @param value The 16-bit signed integer to serialize.
		 * @return ESerializationError Returns InvalidData if serialization fails, otherwise None.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::int16_t value)
		{
			return !m_writer.Int(value) ? ESerializationError::InvalidData : ESerializationError::None;
		}

		/// Serializes an int32 value to a JSON string.
		///
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a 32-bit signed integer as a JSON number.
		 *
		 * @param value The integer value to serialize.
		 * @return ESerializationError None on success, or InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::int32_t value)
		{
			return !m_writer.Int(value) ? ESerializationError::InvalidData : ESerializationError::None;
		}

		/// Serializes an int64 value to a JSON string.
		///
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a 64-bit signed integer as a JSON number.
		 *
		 * @param value The 64-bit signed integer to serialize.
		 * @return ESerializationError None on success, or InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::int64_t value)
		{
			return !m_writer.Int64(value) ? ESerializationError::InvalidData : ESerializationError::None;
		}

		/// Serializes a string value to a JSON string.
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a std::string value as a JSON string.
		 *
		 * @param value The string to serialize.
		 * @return ESerializationError None on success, or InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::string &value)
		{
			return !m_writer.String(value.c_str(), static_cast<rapidjson::SizeType>(value.size()))
					   ? ESerializationError::InvalidData
					   : ESerializationError::None;
		}

		/// Serializes a string_view value to a JSON string.
		/// @param value Value to serialize
		/// @return SerializationError
		template <>
		/**
		 * @brief Serializes a string view as a JSON string value.
		 *
		 * @param value The string view to serialize.
		 * @return ESerializationError None on success, or InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(const std::string_view value)
		{
			return !m_writer.String(value.data(), static_cast<rapidjson::SizeType>(value.size()))
					   ? ESerializationError::InvalidData
					   : ESerializationError::None;
		}

		/// Serializes a span of values to a JSON array.
		/// @param values Value to serialize
		/// @return SerializationError
		template <typename T>
		/**
		 * @brief Serializes a span of values as a JSON array.
		 *
		 * Each element in the span is serialized in sequence. Returns an error if array boundaries cannot be written or if any element fails to serialize.
		 *
		 * @tparam T Type of elements to serialize.
		 * @param values Span of values to serialize as a JSON array.
		 * @return ESerializationError None on success, InvalidData on failure.
		 */
		[[nodiscard]]
		ESerializationError SerializeArray(const std::span<const T> values)
		{
			if (!m_writer.StartArray())
			{
				return ESerializationError::InvalidData;
			}

			for (const auto &value : values)
			{
				if (Serialize(value) == ESerializationError::InvalidData)
				{
					return ESerializationError::InvalidData;
				}
			}

			if (!m_writer.EndArray())
			{
				return ESerializationError::InvalidData;
			}

			return ESerializationError::None;
		}

		/// Serializes a map of values to a JSON object.
		///
		/// @tparam V Value type
		/// @tparam It Iterator type
		/// @param begin Begin iterator
		/// @param end End iterator
		/// @return SerializationError
		template <typename V, IsMapIterator<std::string, V> It>
		/**
		 * @brief Serializes a map of key-value pairs as a JSON object.
		 *
		 * The keys must be serializable as JSON strings, and the values must be serializable by this serializer.
		 *
		 * @tparam It Iterator type pointing to pairs of (key, value).
		 * @param begin Iterator to the beginning of the map.
		 * @param end Iterator to the end of the map.
		 * @return ESerializationError None on success, or InvalidData if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError SerializeMap(It begin, It end)
		{
			if (!m_writer.StartObject())
			{
				return ESerializationError::InvalidData;
			}

			for (auto it = begin; it != end; ++it)
			{
				// Serialize with "key": value
				if (!Serialize(it->first, it->second))
				{
					return ESerializationError::InvalidData;
				}
			}

			return m_writer.EndObject() ? ESerializationError::None : ESerializationError::InvalidData;
		}

		/// Serializes a key-value pair to a JSON object.
		/// @tparam T Type
		/// @param key Key
		/// @param value Value
		/// @return SerializationError
		template <typename T>
		/**
		 * @brief Serializes a key-value pair into the current JSON object.
		 *
		 * Sets the specified key and serializes the associated value. Returns an error if the key is invalid or serialization fails.
		 *
		 * @param key The JSON key to associate with the value.
		 * @param value The value to serialize.
		 * @return ESerializationError None on success, or an error code on failure.
		 */
		[[nodiscard]]
		ESerializationError Serialize(std::string_view key, const T &value)
		{
			if (SetKey(key) != ESerializationError::None)
			{
				return ESerializationError::InvalidData;
			}

			return Serialize(value);
		}

		/// Serializes an array of values to a JSON array.
		///
		/// @tparam T Type
		/// @param key Key
		/// @param values Values
		/// @return SerializationError
		template <typename T>
		/**
		 * @brief Serializes an array of values under the specified JSON key.
		 *
		 * Associates the given key with a JSON array containing the provided values.
		 *
		 * @param key The JSON key to associate with the array.
		 * @param values The array of values to serialize.
		 * @return ESerializationError None on success, or an error code if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(std::string_view key, const std::span<const T> values)
		{
			if (SetKey(key) != ESerializationError::None)
			{
				return ESerializationError::InvalidData;
			}

			return SerializeArray(values);
		}

		/// Serializes a map of values to a JSON object.
		///
		/// @tparam V Value type
		/// @tparam It Iterator type
		/// @param key Key
		/// @param begin Iterator begin
		/// @param end Iterator end
		/// @return SerializationError
		template <typename V, IsMapIterator<std::string, V> It>
		/**
		 * @brief Serializes a map of key-value pairs under a specified JSON key.
		 *
		 * Serializes the range of map elements defined by the iterators [begin, end) as a JSON object, associating it with the given key.
		 *
		 * @param key The JSON key under which the map will be serialized.
		 * @param begin Iterator to the beginning of the map range.
		 * @param end Iterator to the end of the map range.
		 * @return ESerializationError Returns None on success, or an error code if serialization fails.
		 */
		[[nodiscard]]
		ESerializationError Serialize(std::string_view key, It begin, It end)
		{
			if (SetKey(key) != ESerializationError::None)
			{
				return ESerializationError::InvalidData;
			}

			return SerializeMap(begin, end);
		}

		/// Finalizes the serialization process and returns the serialized JSON string.
		/// You should not call any other function after this one.
		///
		/**
		 * @brief Returns the finalized JSON string after serialization.
		 *
		 * No further serialization operations should be performed after calling this method.
		 *
		 * @return Serialized JSON string.
		 */
		std::string FinishSerialization()
		{
			return m_buffer.GetString();
		}

	private:
		rapidjson::StringBuffer m_buffer;
		rapidjson::Writer<rapidjson::StringBuffer> m_writer;
	};

	class JsonDeserializer
	{
		///
		/// RapidjsonVisitor class.
		struct RapidjsonVisitor : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, JsonDeserializer>
		{
			IVisitor *visitor{nullptr};

			/**
			 * @brief Constructs a RapidjsonVisitor with a null visitor pointer.
			 *
			 * Initializes the RapidjsonVisitor handler for use in JSON parsing.
			 */
			RapidjsonVisitor()
			{
			}

			bool Null();

			bool Bool(bool b);

			bool Int(int i);

			bool Uint(unsigned i);

			bool Int64(int64_t i);

			bool Uint64(uint64_t i);

			bool Double(double d);

			bool RawNumber(const Ch *str, rapidjson::SizeType length, bool copy);

			/**
			 * @brief Handles a JSON string value during deserialization.
			 *
			 * Invokes the visitor's string handler with the parsed string value and updates the visitor pointer if successful.
			 *
			 * @param str Pointer to the string data.
			 * @param length Length of the string.
			 * @param copy Unused parameter required by RapidJSON's interface.
			 * @return true if the string was successfully processed by the visitor; false otherwise.
			 */
			bool String(const Ch *str, rapidjson::SizeType length, bool copy)
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

			bool StartObject();

			bool Key(const Ch *str, rapidjson::SizeType length, bool copy);

			bool EndObject(rapidjson::SizeType memberCount);

			bool StartArray();

			bool EndArray(rapidjson::SizeType elementCount);
		};

		static constexpr EFormatDescribingType JSON_DESCRIBING_TYPE = EFormatDescribingType::SelfDescribing;

	public:
		/**
		 * @brief Constructs a JsonDeserializer for the given JSON string.
		 *
		 * Initializes the internal stream with the provided JSON data for subsequent deserialization operations.
		 *
		 * @param json The JSON string to be deserialized.
		 */
		JsonDeserializer(std::string_view json)
			: m_stream(json.data())
		{
		}

		template <typename T>
			requires(BuiltinVisitors::ExistsBuiltinVisitor<T>)
		/**
		 * @brief Deserializes the JSON input into a value of type T using a built-in visitor.
		 *
		 * Uses RapidJSON's SAX parser and a built-in visitor to convert the JSON string into an object of type T.
		 *
		 * @return Result containing the deserialized value or an EDeserializationError if parsing fails.
		 */
		Result<T, EDeserializationError> Deserialize()
		{
			BuiltinVisitors::Visitor<T> visitor(this, JSON_DESCRIBING_TYPE);

			RapidjsonVisitor rapidjsonVisitor;
			rapidjsonVisitor.visitor = &visitor;

			// Parse the JSON string
			rapidjson::ParseResult parseResult = m_reader.Parse(m_stream, rapidjsonVisitor);
			if (parseResult.IsError())
			{
				return EDeserializationError::InvalidData;
			}

			return visitor.value;
		}

		template <typename T>
			requires(IsDeserializable<T>)
		/**
		 * @brief Deserializes the JSON input into an object of type T using its custom deserialization logic.
		 *
		 * @return Result containing the deserialized object on success, or an error code if deserialization fails.
		 */
		[[nodiscard]]
		Result<T, EDeserializationError> Deserialize()
		{
			T finalResult;
			auto visitor = finalResult.Deserialize(JSON_DESCRIBING_TYPE);

			RapidjsonVisitor rapidjsonVisitor;
			rapidjsonVisitor.visitor = &visitor;

			auto result = m_reader.Parse(m_stream, rapidjsonVisitor);
			if (result.IsError())
			{
				return EDeserializationError::InvalidData;
			}

			return finalResult;
		}

	private:
		rapidjson::StringStream m_stream;
		rapidjson::Reader m_reader;
	};

	template <typename T>
	/**
	 * @brief Deserializes a JSON string into an object of type T.
	 *
	 * Attempts to parse the provided JSON string and construct an object of type T. Returns either the deserialized object or an error if parsing fails or the data is invalid.
	 *
	 * @param json The JSON string to deserialize.
	 * @return Result<T, EDeserializationError> The deserialized object or an error code.
	 */
	[[nodiscard]]
	inline Result<T, EDeserializationError> DeserializeJson(std::string_view json)
	{
		JsonDeserializer deserializer(json);

		auto result = deserializer.Deserialize<T>();

		if (result.has_error())
		{
			return EDeserializationError::InvalidData;
		}

		return result.value();
	}

	template <typename T>
	/**
	 * @brief Serializes a value to a JSON string.
	 *
	 * Serializes the given value into a JSON string using the JsonSerializer. Returns either the resulting JSON string or a serialization error.
	 *
	 * @return Result containing the JSON string on success, or an ESerializationError on failure.
	 */
	[[nodiscard]]
	inline Result<std::string, ESerializationError> SerializeJson(const T &value)
	{
		JsonSerializer serializer;

		auto error = serializer.Serialize(value);
		if (error != ESerializationError::None)
		{
			return error;
		}

		return serializer.FinishSerialization();
	}

} // namespace Hush::Serialization