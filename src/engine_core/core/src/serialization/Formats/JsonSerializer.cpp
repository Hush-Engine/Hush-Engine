/*! \file JsonSerializer.cpp
	\author Alan Ramirez
	\date 2025-05-17
	\brief Serialization/deserialization for json
*/

#include "JsonSerializer.hpp"

#include "Assertions.hpp"

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::Null()
{
	if (skipValue)
	{
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		++skipDepth;
		return true;
	}

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
	if (skipValue)
	{
		// We are inside a skipped unknown member; ignore nested keys.
		return true;
	}

	HUSH_ASSERT(visitor != nullptr, "Visitor is null");

	(void)copy;

	IVisitor *previousVisitor = visitor;
	auto result = visitor->VisitKey(std::string_view(str, length));
	if (result.has_error())
	{
		return false;
	}
	visitor = result.value();

	// If the visitor did not route the key to a sub-visitor (it returned the same
	// pointer), the member is unknown to this type, so skip its value. This lets
	// generated (reflection) deserializers safely ignore fields that are not part
	// of the type definition (e.g. serialization metadata such as "id", "key",
	// "__type").
	if (visitor == previousVisitor)
	{
		skipValue = true;
		skipDepth = 0;
	}
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonVisitor::EndObject(rapidjson::SizeType memberCount)
{
	if (skipValue)
	{
		if (skipDepth > 0)
		{
			--skipDepth;
		}
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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
	if (skipValue)
	{
		++skipDepth;
		return true;
	}

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
	if (skipValue)
	{
		if (skipDepth > 0)
		{
			--skipDepth;
		}
		if (skipDepth == 0)
		{
			skipValue = false;
		}
		return true;
	}

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

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::Null()
{
	token = EToken::Null;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::Bool(bool b)
{
	token = EToken::Bool;
	boolValue = b;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::Int(int i)
{
	token = EToken::Int;
	intValue = i;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::Uint(unsigned i)
{
	token = EToken::Uint;
	uintValue = i;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::Int64(int64_t i)
{
	token = EToken::Int64;
	intValue = i;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::Uint64(uint64_t i)
{
	token = EToken::Uint64;
	uintValue = i;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::Double(double d)
{
	token = EToken::Double;
	number = d;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::String(const Ch *str, rapidjson::SizeType length,
																	bool copy)
{
	(void)copy;
	token = EToken::String;
	string.assign(str, length);
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::Key(const Ch *str, rapidjson::SizeType length, bool copy)
{
	(void)copy;
	token = EToken::Key;
	key.assign(str, length);
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::StartObject()
{
	token = EToken::ObjectStart;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::EndObject(rapidjson::SizeType memberCount)
{
	(void)memberCount;
	token = EToken::ObjectEnd;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::StartArray()
{
	token = EToken::ArrayStart;
	return true;
}

bool Hush::Serialization::JsonDeserializer::RapidjsonWalker::EndArray(rapidjson::SizeType elementCount)
{
	(void)elementCount;
	token = EToken::ArrayEnd;
	return true;
}

bool Hush::Serialization::JsonDeserializer::Next()
{
	if (m_peekedToken)
	{
		// A token was buffered by @ref PeekKey; replay it without advancing the reader.
		m_peekedToken = false;

		return m_walkerToken != EToken::EndOfInput && m_walkerToken != EToken::Error;
	}

	if (m_walkerError)
	{
		return false;
	}

	if (!m_walkerInitialized)
	{
		m_reader.IterativeParseInit();
		m_walkerInitialized = true;
	}

	m_walkerToken = EToken::EndOfInput;
	m_walkerKey.clear();
	m_walkerString.clear();

	// IterativeParseNext returns true both when a token was parsed and when the
	// input was exhausted cleanly. Reset the sentinel so we can tell them apart:
	// the walker handler is only invoked when a token was actually produced.
	m_walker.token = EToken::EndOfInput;

	const bool hasToken = m_reader.IterativeParseNext<rapidjson::kParseDefaultFlags>(m_stream, m_walker);
	if (hasToken)
	{
		if (m_walker.token == EToken::EndOfInput)
		{
			// End of input reached without producing a token.
			m_walkerToken = EToken::EndOfInput;
			return false;
		}

		m_walkerToken = m_walker.token;
		switch (m_walkerToken)
		{
		case EToken::ObjectStart:
			++m_walkerObjectDepth;
			break;
		case EToken::ObjectEnd:
			--m_walkerObjectDepth;
			break;
		case EToken::Key:
			m_walkerKey = m_walker.key;
			break;
		case EToken::String:
			m_walkerString = m_walker.string;
			break;
		case EToken::Int:
		case EToken::Int64:
			m_walkerInt = m_walker.intValue;
			break;
		case EToken::Uint:
		case EToken::Uint64:
			m_walkerUint = m_walker.uintValue;
			break;
		case EToken::Double:
			m_walkerNumber = m_walker.number;
			break;
		case EToken::Bool:
			m_walkerBool = m_walker.boolValue;
			break;
		default:
			break;
		}

		return true;
	}

	if (m_reader.HasParseError())
	{
		m_walkerToken = EToken::Error;
		m_walkerError = true;
		return false;
	}

	m_walkerToken = EToken::EndOfInput;
	return false;
}

bool Hush::Serialization::JsonDeserializer::ReadKey(std::string_view &out)
{
	if (!Next())
	{
		return false;
	}

	if (GetToken() != EToken::Key)
	{
		return false;
	}

	out = m_walkerKey;
	return true;
}

bool Hush::Serialization::JsonDeserializer::ReadString(std::string_view &out)
{
	if (!Next())
	{
		return false;
	}

	if (GetToken() != EToken::String)
	{
		return false;
	}

	out = m_walkerString;
	return true;
}

bool Hush::Serialization::JsonDeserializer::ReadDouble(double &out)
{
	if (!Next())
	{
		return false;
	}

	switch (GetToken())
	{
	case EToken::Double:
		out = m_walkerNumber;
		return true;
	case EToken::Int:
	case EToken::Int64:
		out = static_cast<double>(m_walkerInt);
		return true;
	case EToken::Uint:
	case EToken::Uint64:
		out = static_cast<double>(m_walkerUint);
		return true;
	default:
		return false;
	}
}

bool Hush::Serialization::JsonDeserializer::ReadInt(int64_t &out)
{
	if (!Next())
	{
		return false;
	}

	switch (GetToken())
	{
	case EToken::Int:
	case EToken::Int64:
		out = m_walkerInt;
		return true;
	case EToken::Uint:
	case EToken::Uint64:
		out = static_cast<int64_t>(m_walkerUint);
		return true;
	default:
		return false;
	}
}

bool Hush::Serialization::JsonDeserializer::ReadBool(bool &out)
{
	if (!Next())
	{
		return false;
	}

	if (GetToken() != EToken::Bool)
	{
		return false;
	}

	out = m_walkerBool;
	return true;
}

bool Hush::Serialization::JsonDeserializer::ReadNull()
{
	if (!Next())
	{
		return false;
	}

	return GetToken() == EToken::Null;
}

bool Hush::Serialization::JsonDeserializer::SkipValue()
{
	int32_t depth = 0;
	EToken token = GetToken();
	if (token == EToken::ObjectStart || token == EToken::ArrayStart)
	{
		depth = 1;
	}

	while (depth > 0)
	{
		if (!Next())
		{
			return false;
		}

		token = GetToken();
		if (token == EToken::ObjectStart || token == EToken::ArrayStart)
		{
			++depth;
		}
		else if (token == EToken::ObjectEnd || token == EToken::ArrayEnd)
		{
			--depth;
		}
	}

	return true;
}

std::optional<std::string_view> Hush::Serialization::JsonDeserializer::PeekKey()
{
	if (!m_peekedToken)
	{
		// Advance to the next token and buffer it so it can be replayed by @ref Next.
		Next();
		m_peekedToken = true;
	}

	if (m_walkerToken != EToken::Key)
	{
		return std::nullopt;
	}

	return std::string_view(m_walkerKey);
}

bool Hush::Serialization::JsonDeserializer::PeekObject(std::string_view &out)
{
	if (!m_peekedToken)
	{
		// Advance to the next token and buffer it so it can be replayed if it is
		// not an object start.
		if (!Next())
		{
			return false;
		}
		m_peekedToken = true;
	}

	if (GetToken() != EToken::ObjectStart)
	{
		// Leave the token buffered so a later @ref Next / @ref ReadKey consumes it.
		return false;
	}

	// The opening brace was just consumed (or peeked), so the stream is one byte
	// past it. The reader has not consumed any of the object's contents, so scan
	// the raw input directly to find the matching closing brace without moving
	// the walker.
	const size_t begin = m_stream.Tell() - 1;

	size_t closingBrace = begin;
	int32_t depth = 0;
	bool inString = false;
	bool escaped = false;
	while (closingBrace < m_input.size())
	{
		const char c = m_input[closingBrace];
		if (inString)
		{
			if (escaped)
			{
				escaped = false;
			}
			else if (c == '\\')
			{
				escaped = true;
			}
			else if (c == '"')
			{
				inString = false;
			}
		}
		else
		{
			if (c == '"')
			{
				inString = true;
			}
			else if (c == '{')
			{
				++depth;
			}
			else if (c == '}' && --depth == 0)
			{
				out = std::string_view(m_input.data() + begin, closingBrace - begin + 1);
				return true;
			}
		}

		++closingBrace;
	}

	// No matching closing brace found.
	return false;
}

bool Hush::Serialization::JsonDeserializer::SkipObject()
{
	if (GetToken() == EToken::ObjectEnd)
	{
		// Already at the end of the current object scope.
		return true;
	}

	if (GetToken() == EToken::None || GetToken() == EToken::EndOfInput || GetToken() == EToken::Error)
	{
		return false;
	}

	if (m_walkerObjectDepth == 0)
	{
		// The walker is not inside any object scope.
		return false;
	}

	// Skip the innermost open object scope that contains the current token. The
	// depth counter already reflects the current token (whether it was pulled by
	// @ref Next or buffered by @ref PeekKey/@ref PeekObject), so consume tokens
	// until the enclosing object's closing brace brings the depth back down.
	const int32_t targetDepth = m_walkerObjectDepth;

	// If the current token was peeked, consume the buffered token.
	m_peekedToken = false;

	while (m_walkerObjectDepth >= targetDepth)
	{
		if (!Next())
		{
			return false;
		}
	}

	return true;
}

bool Hush::Serialization::JsonDeserializer::ReadObject(std::string_view &out)
{
	if (!PeekObject(out))
	{
		return false;
	}

	return SkipObject();
}