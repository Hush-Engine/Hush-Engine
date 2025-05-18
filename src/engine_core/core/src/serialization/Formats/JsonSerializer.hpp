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
		JsonSerializer()
			: m_writer(m_buffer)
		{
		}

		/// Sets the key for the next value to be serialized.
		/// @param key Key to serialize
		/// @return SerializationError
		[[nodiscard]]
		ESerializationError SetKey(std::string_view key)
		{
			return m_writer.Key(key.data(), static_cast<rapidjson::SizeType>(key.size()))
					   ? ESerializationError::None
					   : ESerializationError::InvalidData;
		}

		/// Begins an object in the JSON string.
		/// @return SerializationError
		[[nodiscard]]
		ESerializationError BeginObject()
		{
			return m_writer.StartObject() ? ESerializationError::None : ESerializationError::InvalidData;
		}

		/// Ends an object in the JSON string.
		/// @return SerializationError
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
		ESerializationError Serialize(const T &)
		{
			static_assert(false, "Type is not serializable");

			return ESerializationError::InvalidType;
		}

		template <typename T>
			requires(sizeof(T) <= 16 && !IsSerializable<T, JsonSerializer>)
		[[nodiscard]]
		ESerializationError Serialize(const T)
		{
			static_assert(false, "Type is not serializable");

			return ESerializationError::InvalidType;
		}

		/// Serializes a span of values to a JSON array.
		/// @param values Value to serialize
		/// @return SerializationError
		template <typename T>
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
				if (Serialize(it->first, it->second) != ESerializationError::None)
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
		/// @return Serialized JSON string
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

			bool String(const Ch *str, rapidjson::SizeType length, bool copy);

			bool StartObject();

			bool Key(const Ch *str, rapidjson::SizeType length, bool copy);

			bool EndObject(rapidjson::SizeType memberCount);

			bool StartArray();

			bool EndArray(rapidjson::SizeType elementCount);
		};

		static constexpr EFormatDescribingType JSON_DESCRIBING_TYPE = EFormatDescribingType::SelfDescribing;

	public:
		JsonDeserializer(std::string_view json)
			: m_stream(json.data())
		{
		}

		template <typename T>
			requires(BuiltinVisitors::ExistsBuiltinVisitor<T>)
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

	/// Serializes a double value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	inline ESerializationError JsonSerializer::Serialize<double>(const double value)
	{
		return m_writer.Double(value) ? ESerializationError::None : ESerializationError::InvalidData;
	}

	/// Serializes a float value to a JSON string.
	///
	/// @param key Key to serialize
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const float value)
	{
		return Serialize<double>(value);
	}

	/// Serializes a boolean value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const bool value)
	{
		return m_writer.Bool(value) ? ESerializationError::None : ESerializationError::InvalidData;
	}

	/// Serializes an uint8 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::uint8_t value)
	{
		return !m_writer.Uint(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an uint16 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::uint16_t value)
	{
		return !m_writer.Uint(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an uint32 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::uint32_t value)
	{
		return !m_writer.Uint(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an uint64 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::uint64_t value)
	{
		return !m_writer.Uint64(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an int8 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::int8_t value)
	{
		return !m_writer.Int(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an int16 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::int16_t value)
	{
		return !m_writer.Int(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an int32 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::int32_t value)
	{
		return !m_writer.Int(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an int64 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::int64_t value)
	{
		return !m_writer.Int64(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes a string value to a JSON string.
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::string &value)
	{
		return !m_writer.String(value.c_str(), static_cast<rapidjson::SizeType>(value.size()))
				   ? ESerializationError::InvalidData
				   : ESerializationError::None;
	}

	/// Serializes a string_view value to a JSON string.
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::string_view value)
	{
		return !m_writer.String(value.data(), static_cast<rapidjson::SizeType>(value.size()))
				   ? ESerializationError::InvalidData
				   : ESerializationError::None;
	}

	template <typename T>
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