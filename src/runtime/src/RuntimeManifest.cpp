/*! \file RuntimeManifest.cpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Manifest of an exported runtime, describes content and modules
*/

#include "RuntimeManifest.hpp"

#include "serialization/Formats/JsonSerializer.hpp"

#include <fstream>
#include <optional>
#include <sstream>

namespace
{
	using JsonDeserializer = Hush::Serialization::JsonDeserializer;
	using JsonToken = JsonDeserializer::EToken;

	bool SkipMemberValue(JsonDeserializer &deserializer)
	{
		return deserializer.Next() && deserializer.SkipValue();
	}

	bool FinishDocument(JsonDeserializer &deserializer, JsonToken closingToken)
	{
		if (!deserializer.Next() || deserializer.GetToken() != closingToken)
		{
			return false;
		}
		return !deserializer.Next() && !deserializer.HasError() && deserializer.GetToken() == JsonToken::EndOfInput;
	}

	bool ReadOptionalString(JsonDeserializer &deserializer, std::string &value)
	{
		if (!deserializer.Next())
		{
			return false;
		}
		if (deserializer.GetToken() == JsonToken::String)
		{
			value = deserializer.GetString();
			return true;
		}
		return deserializer.SkipValue();
	}

	bool ParseModule(std::string_view json, Hush::RuntimeModuleManifest &module)
	{
		JsonDeserializer deserializer(json);
		if (!deserializer.Next() || deserializer.GetToken() != JsonToken::ObjectStart)
		{
			return false;
		}

		while (deserializer.PeekKey().has_value())
		{
			std::string_view key;
			if (!deserializer.ReadKey(key))
			{
				return false;
			}

			if (key == "name")
			{
				if (!ReadOptionalString(deserializer, module.name))
				{
					return false;
				}
			}
			else if (key == "kind")
			{
				if (!ReadOptionalString(deserializer, module.kind))
				{
					return false;
				}
			}
			else if (key == "path")
			{
				if (!ReadOptionalString(deserializer, module.path))
				{
					return false;
				}
			}
			else if (key == "assembly")
			{
				if (!ReadOptionalString(deserializer, module.assembly))
				{
					return false;
				}
			}
			else if (key == "runtimeConfig")
			{
				if (!ReadOptionalString(deserializer, module.runtimeConfig))
				{
					return false;
				}
			}
			else if (!SkipMemberValue(deserializer))
			{
				return false;
			}
		}

		return FinishDocument(deserializer, JsonToken::ObjectEnd);
	}

	bool ParseModules(std::string_view json, std::vector<Hush::RuntimeModuleManifest> &modules)
	{
		JsonDeserializer deserializer(json);
		if (!deserializer.Next() || deserializer.GetToken() != JsonToken::ArrayStart)
		{
			return false;
		}

		while (true)
		{
			std::string_view moduleJson;
			if (deserializer.ReadObject(moduleJson))
			{
				Hush::RuntimeModuleManifest module;
				const std::string ownedJson(moduleJson);
				if (!ParseModule(ownedJson, module))
				{
					return false;
				}
				modules.push_back(std::move(module));
				continue;
			}

			if (!deserializer.Next() || deserializer.GetToken() != JsonToken::ArrayEnd)
			{
				return false;
			}
			return !deserializer.Next() && !deserializer.HasError() && deserializer.GetToken() == JsonToken::EndOfInput;
		}
	}

	bool ParseSystem(std::string_view json, Hush::RuntimeSystemManifest &system)
	{
		JsonDeserializer deserializer(json);
		if (!deserializer.Next() || deserializer.GetToken() != JsonToken::ObjectStart)
		{
			return false;
		}

		while (deserializer.PeekKey().has_value())
		{
			std::string_view key;
			if (!deserializer.ReadKey(key))
			{
				return false;
			}

			if (key == "module")
			{
				if (!ReadOptionalString(deserializer, system.module))
				{
					return false;
				}
			}
			else if (key == "type")
			{
				if (!ReadOptionalString(deserializer, system.type))
				{
					return false;
				}
			}
			else if (!SkipMemberValue(deserializer))
			{
				return false;
			}
		}

		return FinishDocument(deserializer, JsonToken::ObjectEnd);
	}

	bool ParseSystems(std::string_view json, std::vector<Hush::RuntimeSystemManifest> &systems)
	{
		JsonDeserializer deserializer(json);
		if (!deserializer.Next() || deserializer.GetToken() != JsonToken::ArrayStart)
		{
			return false;
		}

		while (true)
		{
			std::string_view systemJson;
			if (deserializer.ReadObject(systemJson))
			{
				Hush::RuntimeSystemManifest system;
				const std::string ownedJson(systemJson);
				if (!ParseSystem(ownedJson, system))
				{
					return false;
				}
				systems.push_back(std::move(system));
				continue;
			}

			if (!deserializer.Next() || deserializer.GetToken() != JsonToken::ArrayEnd)
			{
				return false;
			}
			return !deserializer.Next() && !deserializer.HasError() && deserializer.GetToken() == JsonToken::EndOfInput;
		}
	}
} // namespace

Hush::Result<Hush::RuntimeManifest, Hush::RuntimeManifest::EError> Hush::RuntimeManifest::LoadFromFile(
	const std::filesystem::path &path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file.is_open())
	{
		return EError::FileNotFound;
	}

	std::stringstream buffer;
	buffer << file.rdbuf();
	return Parse(buffer.str());
}

Hush::Result<Hush::RuntimeManifest, Hush::RuntimeManifest::EError> Hush::RuntimeManifest::Parse(std::string_view json)
{
	JsonDeserializer deserializer(json);
	if (!deserializer.Next() || deserializer.GetToken() != JsonToken::ObjectStart)
	{
		return EError::ParseError;
	}

	RuntimeManifest manifest;
	bool hasFormatVersion = false;
	std::optional<std::string> modules;
	std::optional<std::string> systems;
	while (deserializer.PeekKey().has_value())
	{
		std::string_view key;
		if (!deserializer.ReadKey(key))
		{
			return EError::ParseError;
		}

		if (key == "formatVersion")
		{
			if (!deserializer.Next() || deserializer.GetToken() != JsonToken::Uint)
			{
				return EError::ParseError;
			}
			manifest.formatVersion = static_cast<std::uint32_t>(deserializer.GetUint());
			hasFormatVersion = true;
		}
		else if (key == "name")
		{
			if (!ReadOptionalString(deserializer, manifest.name))
			{
				return EError::ParseError;
			}
		}
		else if (key == "startupScene")
		{
			if (!ReadOptionalString(deserializer, manifest.startupScene))
			{
				return EError::ParseError;
			}
		}
		else if (key == "content")
		{
			if (!ReadOptionalString(deserializer, manifest.content))
			{
				return EError::ParseError;
			}
		}
		else if (key == "modules")
		{
			std::string_view value;
			if (deserializer.ReadArray(value))
			{
				modules = std::string(value);
			}
			else if (!SkipMemberValue(deserializer))
			{
				return EError::ParseError;
			}
		}
		else if (key == "systems")
		{
			std::string_view value;
			if (deserializer.ReadArray(value))
			{
				systems = std::string(value);
			}
			else if (!SkipMemberValue(deserializer))
			{
				return EError::ParseError;
			}
		}
		else if (!SkipMemberValue(deserializer))
		{
			return EError::ParseError;
		}
	}

	if (!FinishDocument(deserializer, JsonToken::ObjectEnd) || !hasFormatVersion)
	{
		return EError::ParseError;
	}
	if (manifest.formatVersion == 0 || manifest.formatVersion > CURRENT_VERSION)
	{
		return EError::UnsupportedVersion;
	}
	if ((modules.has_value() && !ParseModules(*modules, manifest.modules)) ||
		(systems.has_value() && !ParseSystems(*systems, manifest.systems)))
	{
		return EError::ParseError;
	}

	return manifest;
}
