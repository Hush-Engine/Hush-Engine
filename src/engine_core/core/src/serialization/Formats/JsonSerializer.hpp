/*! \file JsonSerializer.hpp
	\author Alan Ramirez
	\date 2025-05-17
	\brief Serialization/deserialization for json
*/

#pragma once

#include "Assertions.hpp"
#include <Result.hpp>
#include <rapidjson/memorystream.h>
#include <rapidjson/rapidjson.h>
#include <rapidjson/writer.h>
#include <rapidjson/reader.h>
#include <string_view>
#include <span>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../Serialization.hpp"
#include "../Deserialization.hpp"

// BACKLOG: Assisted by AI, needs review

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

		[[nodiscard]]
		ESerializationError BeginArray()
		{
			return this->m_writer.StartArray() ? ESerializationError::None : ESerializationError::InvalidData;
		}

		[[nodiscard]]
		ESerializationError EndArray()
		{
			return this->m_writer.EndArray() ? ESerializationError::None : ESerializationError::InvalidData;
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
		ESerializationError Serialize(const T &value, bool wrapInObject = true)
		{
			if (wrapInObject)
			{
				m_writer.StartObject();
				ESerializationError error = value.Serialize(*this);
				m_writer.EndObject();
				return error;
			}

			ESerializationError error = value.Serialize(*this);
			return error;
		}

		template <typename T>
			requires(!IsSerializable<T, JsonSerializer>)
		[[nodiscard]]
		ESerializationError Serialize(const T &)
		{
			static_assert(false, "Type is not serializable");

			return ESerializationError::InvalidType;
		}

		/// Serializes a GLM vector to a JSON array of floats (contiguous).
		/// @tparam L Vector length
		/// @tparam Q Vector qualifier
		/// @param value Value to serialize
		/// @return SerializationError
		template <glm::length_t L, glm::qualifier Q>
		[[nodiscard]]
		ESerializationError Serialize(const glm::vec<L, float, Q> &value)
		{
			if (BeginArray() != ESerializationError::None)
			{
				return ESerializationError::InvalidData;
			}

			const float *data = glm::value_ptr(value);
			for (glm::length_t i = 0; i < L; ++i)
			{
				if (Serialize(data[i]) != ESerializationError::None)
				{
					return ESerializationError::InvalidData;
				}
			}

			return EndArray();
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

		/// Serializes a vector of values to a JSON array.
		///
		/// @tparam T Element type
		/// @param key Key
		/// @param values Values
		/// @return SerializationError
		template <typename T>
		[[nodiscard]]
		ESerializationError Serialize(std::string_view key, const std::vector<T> &values)
		{
			if (SetKey(key) != ESerializationError::None)
			{
				return ESerializationError::InvalidData;
			}

			if (!m_writer.StartArray())
			{
				return ESerializationError::InvalidData;
			}

			for (const auto &value : values)
			{
				if (Serialize(value) != ESerializationError::None)
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
	public:
		/// Token type produced by the raw, type-erased walking API (@ref Next).
		enum class EToken
		{
			/// No token has been read yet (before the first call to @ref Next).
			None,
			/// Start of an object (`{`).
			ObjectStart,
			/// End of an object (`}`).
			ObjectEnd,
			/// Start of an array (`[`).
			ArrayStart,
			/// End of an array (`]`).
			ArrayEnd,
			/// An object member key.
			Key,
			/// A string value.
			String,
			/// A signed integer value.
			Int,
			/// An unsigned integer value.
			Uint,
			/// A signed integer value that does not fit in an int32.
			Int64,
			/// An unsigned integer value that does not fit in a uint32.
			Uint64,
			/// A floating point value.
			Double,
			/// A boolean value.
			Bool,
			/// A null value.
			Null,
			/// The end of the JSON input was reached.
			EndOfInput,
			/// A parse error occurred.
			Error,
		};

	private:
		///
		/// RapidjsonVisitor class.
		struct RapidjsonVisitor : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, JsonDeserializer>
		{
			IVisitor *visitor{nullptr};

			/// Whether the value of an unknown object member is being skipped. When a
			/// visitor's @ref IVisitor::VisitKey returns the same visitor (i.e. it did
			/// not route the key to a sub-visitor), the member is considered unknown
			/// and its value — scalar or nested container — is consumed without being
			/// forwarded. This lets generated (reflection) deserializers safely
			/// ignore fields that are not part of the type definition.
			bool skipValue{false};

			/// Nesting depth while skipping an unknown container value.
			int32_t skipDepth{0};

			RapidjsonVisitor() = default;

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

		///
		/// RapidjsonWalker class.
		/// A raw, pull-based token reader backed by rapidjson's iterative parser.
		struct RapidjsonWalker : public rapidjson::BaseReaderHandler<rapidjson::UTF8<>, RapidjsonWalker>
		{
			EToken token = EToken::EndOfInput;
			std::string key;
			std::string string;
			double number = 0.0;
			int64_t intValue = 0;
			uint64_t uintValue = 0;
			bool boolValue = false;

			bool Null();

			bool Bool(bool b);

			bool Int(int i);

			bool Uint(unsigned i);

			bool Int64(int64_t i);

			bool Uint64(uint64_t i);

			bool Double(double d);

			bool String(const Ch *str, rapidjson::SizeType length, bool copy);

			bool Key(const Ch *str, rapidjson::SizeType length, bool copy);

			bool StartObject();

			bool EndObject(rapidjson::SizeType memberCount);

			bool StartArray();

			bool EndArray(rapidjson::SizeType elementCount);
		};

		static constexpr EFormatDescribingType JSON_DESCRIBING_TYPE = EFormatDescribingType::SelfDescribing;

	public:
		JsonDeserializer(std::string_view json)
			: m_input(json),
			  m_stream(json.data(), json.size())
		{
		}

		template <typename T>
			requires(BuiltinVisitors::ExistsBuiltinVisitor<T>)
		Result<T, EDeserializationError> Deserialize()
		{
			BuiltinVisitors::Visitor<T> visitor(nullptr, JSON_DESCRIBING_TYPE);

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
			auto visitor = finalResult.Deserialize(nullptr, JSON_DESCRIBING_TYPE);

			RapidjsonVisitor rapidjsonVisitor;
			rapidjsonVisitor.visitor = visitor.GetStartVisitor();

			auto result = m_reader.Parse(m_stream, rapidjsonVisitor);
			if (result.IsError())
			{
				return EDeserializationError::InvalidData;
			}

			return finalResult;
		}

		template <typename T>
			requires(IsDeserializable<T>)
		[[nodiscard]]
		EDeserializationError Deserialize(T *instance)
		{
			HUSH_ASSERT(instance != nullptr, "Instance of object passed to deserialize cannot be null!");
			auto visitor = instance->Deserialize(nullptr, JSON_DESCRIBING_TYPE);

			RapidjsonVisitor rapidjsonVisitor;
			rapidjsonVisitor.visitor = visitor.GetStartVisitor();

			auto result = m_reader.Parse(m_stream, rapidjsonVisitor);
			if (result.IsError())
			{
				return EDeserializationError::InvalidData;
			}

			return EDeserializationError::None;
		}

		// ── Raw, type-erased walking API ───────────────────────────────────────

		/// Advances the reader to the next token.
		///
		/// @return false when the input has been exhausted or a parse error occurred
		///         (check @ref HasError to distinguish the two).
		bool Next();

		/// The token produced by the most recent call to @ref Next.
		[[nodiscard]]
		EToken GetToken() const
		{
			return m_walkerToken;
		}

		/// The current object member key. Valid until the next call to @ref Next.
		[[nodiscard]]
		std::string_view GetKey() const
		{
			return m_walkerKey;
		}

		/// The current string value. Valid until the next call to @ref Next.
		[[nodiscard]]
		std::string_view GetString() const
		{
			return m_walkerString;
		}

		/// The current floating point value. Integer tokens are converted to a double.
		[[nodiscard]]
		double GetDouble() const
		{
			switch (m_walkerToken)
			{
			case EToken::Double:
				return m_walkerNumber;
			case EToken::Int:
			case EToken::Int64:
				return static_cast<double>(m_walkerInt);
			case EToken::Uint:
			case EToken::Uint64:
				return static_cast<double>(m_walkerUint);
			default:
				return 0.0;
			}
		}

		/// The current signed integer value.
		[[nodiscard]]
		int64_t GetInt() const
		{
			return m_walkerInt;
		}

		/// The current unsigned integer value.
		[[nodiscard]]
		uint64_t GetUint() const
		{
			return m_walkerUint;
		}

		/// The current boolean value.
		[[nodiscard]]
		bool GetBool() const
		{
			return m_walkerBool;
		}

		/// Reads the next token as an object member key.
		/// @return false if the next token is not a key.
		bool ReadKey(std::string_view &out);

		/// Reads the next token as a string value.
		/// @return false if the next token is not a string.
		bool ReadString(std::string_view &out);

		/// Reads the next token as a floating point value.
		/// Integer tokens are accepted as well.
		/// @return false if the next token is not a number.
		bool ReadDouble(double &out);

		/// Reads the next token as a signed integer value.
		/// @return false if the next token is not an integer.
		bool ReadInt(int64_t &out);

		/// Reads the next token as a boolean value.
		/// @return false if the next token is not a boolean.
		bool ReadBool(bool &out);

		/// Reads the next token as a null value.
		/// @return false if the next token is not null.
		bool ReadNull();

		/// Skips the value at the current token. If the value is an object or an
		/// array, its entire contents are skipped.
		/// @return false on parse error or end of input.
		bool SkipValue();

		/// Peeks the next object member key without consuming it.
		///
		/// The walker is left in place: the token is buffered so a subsequent call
		/// to @ref Next (or @ref ReadKey) returns the same key again. Repeated calls
		/// to @ref PeekKey return the same result until the token is consumed.
		///
		/// The returned view is valid until the walker advances past this token.
		/// @return the next key, or std::nullopt if the next token is not a key or
		///         the input has ended/errored.
		std::optional<std::string_view> PeekKey();

		/// Peeks the next token as an object scope.
		///
		/// If the next token is an object start, @p out is set to the raw JSON of
		/// that object, including its opening and closing braces (e.g. `{ "x": 1 }`).
		/// The view points into the original input buffer and remains valid while
		/// that buffer is alive.
		///
		/// Like @ref PeekKey this is non-destructive: the walker is left in place
		/// with the object-start token buffered, so a subsequent @ref Next returns
		/// the same `ObjectStart` token and the object's contents can be walked
		/// afterwards. Repeated calls to @ref PeekObject return the same view.
		/// If the next token is not an object start, @p out is left untouched, the
		/// token is buffered (a subsequent @ref Next / @ref ReadKey returns it), and
		/// this function returns false.
		///
		/// The returned view can be passed directly to another @ref JsonDeserializer
		/// while the original input buffer remains alive.
		/// @return true and sets @p out on success, false otherwise.
		bool PeekObject(std::string_view &out);

		/// Skips the current object scope.
		///
		/// If the walker is inside an object — whether the current token is the
		/// object's start (pulled by @ref Next or buffered by @ref PeekKey/
		/// @ref PeekObject) or any token in the middle of the object — the rest of
		/// the innermost enclosing object scope is consumed, including nested
		/// objects and arrays. The walker is left positioned on the token that
		/// follows that object's closing brace.
		///
		/// If the walker is not inside any object scope, the walker is left in place
		/// and this function returns false.
		/// @return true if an object scope was skipped, false otherwise.
		bool SkipObject();

		/// Reads the next object scope, returning its raw JSON and advancing the walker.
		///
		/// Combines @ref PeekObject and @ref SkipObject: if the next token is an
		/// object start, @p out is set to the raw JSON of that object (including its
		/// braces, same view @ref PeekObject would return), and the object is then
		/// consumed so the walker is left positioned on the token that follows the
		/// object's closing brace.
		///
		/// The returned view points into the original input buffer and remains valid
		/// while that buffer is alive and can be passed directly to another
		/// @ref JsonDeserializer.
		/// If the next token is not an object start, @p out is left untouched, the
		/// token is buffered (a subsequent @ref Next / @ref ReadKey returns it), and
		/// this function returns false.
		/// @return true and sets @p out on success, false otherwise.
		bool ReadObject(std::string_view &out);

		/// Peeks the next token as an array scope.
		///
		/// The array counterpart of @ref PeekObject: if the next token is an array
		/// start, @p out is set to the raw JSON of that array, including its opening
		/// and closing brackets (e.g. `[1, 2, 3]`), without consuming it. The walker
		/// is left in place with the array-start token buffered, so a subsequent
		/// @ref Next returns the same `ArrayStart` token. Repeated calls return the
		/// same view.
		///
		/// The returned view can be passed directly to another @ref JsonDeserializer
		/// while the original input buffer remains alive.
		/// @return true and sets @p out on success, false otherwise.
		bool PeekArray(std::string_view &out);

		/// Skips the current array scope.
		///
		/// The array counterpart of @ref SkipObject: if the walker is inside an
		/// array — whether the current token is the array's start or any token in
		/// the middle of it — the rest of the innermost enclosing array scope is
		/// consumed, including nested arrays and objects. The walker is left
		/// positioned on the token that follows that array's closing bracket.
		///
		/// If the walker is not inside any array scope, the walker is left in place
		/// and this function returns false.
		/// @return true if an array scope was skipped, false otherwise.
		bool SkipArray();

		/// Reads the next array scope, returning its raw JSON and advancing the walker.
		///
		/// Combines @ref PeekArray and @ref SkipArray: if the next token is an array
		/// start, @p out is set to the raw JSON of that array (including its
		/// brackets, same view @ref PeekArray would return), and the array is then
		/// consumed so the walker is left positioned on the token that follows the
		/// array's closing bracket.
		///
		/// The returned view points into the original input buffer and remains valid
		/// while that buffer is alive and can be passed directly to another
		/// @ref JsonDeserializer.
		/// @return true and sets @p out on success, false otherwise.
		bool ReadArray(std::string_view &out);

		/// @return true if a parse error occurred while walking the JSON.
		[[nodiscard]]
		bool HasError() const
		{
			return m_walkerError;
		}

		/// The rapidjson error code of the last parse error.
		[[nodiscard]]
		rapidjson::ParseErrorCode GetErrorCode() const
		{
			return m_reader.GetParseErrorCode();
		}

		/// The byte offset of the last parse error in the input.
		[[nodiscard]]
		size_t GetErrorOffset() const
		{
			return m_reader.GetErrorOffset();
		}

	private:
		std::string_view m_input;
		rapidjson::MemoryStream m_stream;
		rapidjson::Reader m_reader;
		RapidjsonWalker m_walker;
		EToken m_walkerToken = EToken::None;
		std::string m_walkerKey;
		std::string m_walkerString;
		double m_walkerNumber = 0.0;
		int64_t m_walkerInt = 0;
		uint64_t m_walkerUint = 0;
		bool m_walkerBool = false;
		bool m_walkerError = false;
		bool m_walkerInitialized = false;
		bool m_peekedToken = false;
		int32_t m_walkerObjectDepth = 0;
		int32_t m_walkerArrayDepth = 0;
	};

	/// Serializes a double value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	inline ESerializationError JsonSerializer::Serialize<double>(const double &value)
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
	inline ESerializationError JsonSerializer::Serialize(const float &value)
	{
		const double promoted = value;
		return Serialize<double>(promoted);
	}

	/// Serializes a boolean value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const bool &value)
	{
		return m_writer.Bool(value) ? ESerializationError::None : ESerializationError::InvalidData;
	}

	/// Serializes an uint8 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::uint8_t &value)
	{
		return !m_writer.Uint(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an uint16 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::uint16_t &value)
	{
		return !m_writer.Uint(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an uint32 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::uint32_t &value)
	{
		return !m_writer.Uint(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an uint64 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::uint64_t &value)
	{
		return !m_writer.Uint64(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an int8 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::int8_t &value)
	{
		return !m_writer.Int(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an int16 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::int16_t &value)
	{
		return !m_writer.Int(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an int32 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::int32_t &value)
	{
		return !m_writer.Int(value) ? ESerializationError::InvalidData : ESerializationError::None;
	}

	/// Serializes an int64 value to a JSON string.
	///
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const std::int64_t &value)
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
	inline ESerializationError JsonSerializer::Serialize(const std::string_view &value)
	{
		return !m_writer.String(value.data(), static_cast<rapidjson::SizeType>(value.size()))
				   ? ESerializationError::InvalidData
				   : ESerializationError::None;
	}

	/// Serializes a 4x4 matrix to a JSON array of 16 floats (column-major, contiguous).
	/// @param value Value to serialize
	/// @return SerializationError
	template <>
	[[nodiscard]]
	inline ESerializationError JsonSerializer::Serialize(const glm::mat4 &value)
	{
		if (BeginArray() != ESerializationError::None)
		{
			return ESerializationError::InvalidData;
		}

		const float *data = glm::value_ptr(value);
		for (std::size_t i = 0; i < 16; ++i)
		{
			if (Serialize(data[i]) != ESerializationError::None)
			{
				return ESerializationError::InvalidData;
			}
		}

		return EndArray();
	}

	template <typename T>
	[[nodiscard]]
	inline Result<T, EDeserializationError> DeserializeJson(std::string_view json)
	{
		JsonDeserializer deserializer(json);

		auto result = deserializer.Deserialize<T>();

		if (result.has_error())
		{
			return result.error();
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
