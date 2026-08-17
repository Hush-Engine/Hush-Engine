#include "HMeta.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

namespace Hush
{

	Result<std::string, Serialization::ESerializationError> HMeta::ToJson() const
	{
		rapidjson::StringBuffer buffer;
		rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);

		writer.StartObject();

		writer.Key("version");
		writer.Uint(this->version);
		writer.Key("id");
		writer.Uint64(this->id);
		writer.Key("sourceHash");
		writer.Uint64(this->sourceHash);
		writer.Key("sourceMTime");
		writer.Uint64(this->sourceMTime);
		writer.Key("assetType");
		writer.String(this->assetType.c_str());
		writer.Key("outputFormat");
		writer.Uint(static_cast<uint32_t>(this->outputFormat));
		writer.Key("compression");
		switch (this->compression)
		{
		case ECompressionFormat::Zstd:
			writer.String("zstd");
			break;
		default:
			writer.String("none");
			break;
		}

		writer.Key("importSettings");
		writer.StartObject();

		// Texture settings
		writer.Key("sRGB");
		writer.Bool(this->texture.sRGB);
		writer.Key("generateMipmaps");
		writer.Bool(this->texture.generateMipmaps);
		writer.Key("gpuCompression");
		writer.String(this->texture.gpuCompression.c_str());
		writer.Key("maxSize");
		writer.Uint(this->texture.maxSize);

		// Shader settings
		writer.Key("backends");
		writer.StartArray();
		for (auto b : this->shader.backends)
			writer.Uint(static_cast<uint32_t>(b));
		writer.EndArray();

		writer.Key("entryPoints");
		writer.StartArray();
		for (const auto &ep : this->shader.entryPoints)
		{
			if (ep.stage.empty())
			{
				// Name-only form (stage inferred downstream).
				writer.String(ep.name.c_str());
			}
			else
			{
				writer.StartObject();
				writer.Key("name");
				writer.String(ep.name.c_str());
				writer.Key("stage");
				writer.String(ep.stage.c_str());
				writer.EndObject();
			}
		}
		writer.EndArray();

		writer.Key("defines");
		writer.StartArray();
		for (const auto &d : this->shader.defines)
			writer.String(d.c_str());
		writer.EndArray();

		// Model settings
		writer.Key("importMaterials");
		writer.Bool(this->model.importMaterials);
		writer.Key("generateLods");
		writer.Bool(this->model.generateLods);
		writer.Key("scaleFactor");
		writer.Double(static_cast<double>(this->model.scaleFactor));

		writer.EndObject(); // importSettings
		writer.EndObject(); // root

		return std::string(buffer.GetString(), buffer.GetSize());
	}

	Result<HMeta, Serialization::EDeserializationError> HMeta::FromJson(std::string_view json)
	{
		rapidjson::Document doc;
		doc.Parse(json.data(), json.size());

		if (doc.HasParseError())
		{
			return Serialization::EDeserializationError::InvalidFormat;
		}

		if (!doc.IsObject())
		{
			return Serialization::EDeserializationError::InvalidData;
		}

		HMeta meta;

		// Read top-level fields
		if (doc.HasMember("version") && doc["version"].IsUint())
			meta.version = static_cast<uint16_t>(doc["version"].GetUint());
		if (doc.HasMember("id") && doc["id"].IsUint())
			meta.id = doc["id"].GetUint();
		if (doc.HasMember("sourceHash") && doc["sourceHash"].IsUint64())
			meta.sourceHash = doc["sourceHash"].GetUint64();
		if (doc.HasMember("sourceMTime") && doc["sourceMTime"].IsUint64())
			meta.sourceMTime = doc["sourceMTime"].GetUint64();
		if (doc.HasMember("assetType") && doc["assetType"].IsString())
			meta.assetType = doc["assetType"].GetString();
		if (doc.HasMember("outputFormat") && doc["outputFormat"].IsUint())
			meta.outputFormat = static_cast<EAssetFormat>(doc["outputFormat"].GetUint());
		if (doc.HasMember("compression"))
		{
			if (doc["compression"].IsString())
			{
				std::string_view compStr = doc["compression"].GetString();
				if (compStr == "zstd")
					meta.compression = ECompressionFormat::Zstd;
				else
					meta.compression = ECompressionFormat::None;
			}
			else if (doc["compression"].IsUint())
			{
				meta.compression = static_cast<ECompressionFormat>(doc["compression"].GetUint());
			}
		}

		// Read import settings
		if (doc.HasMember("importSettings") && doc["importSettings"].IsObject())
		{
			const auto &settings = doc["importSettings"];

			// Texture settings
			if (settings.HasMember("sRGB") && settings["sRGB"].IsBool())
				meta.texture.sRGB = settings["sRGB"].GetBool();
			if (settings.HasMember("generateMipmaps") && settings["generateMipmaps"].IsBool())
				meta.texture.generateMipmaps = settings["generateMipmaps"].GetBool();
			if (settings.HasMember("gpuCompression") && settings["gpuCompression"].IsString())
				meta.texture.gpuCompression = settings["gpuCompression"].GetString();
			if (settings.HasMember("maxSize") && settings["maxSize"].IsUint())
				meta.texture.maxSize = settings["maxSize"].GetUint();

			// Shader settings
			if (settings.HasMember("backends") && settings["backends"].IsArray())
			{
				meta.shader.backends.clear();
				for (auto &v : settings["backends"].GetArray())
				{
					if (v.IsUint())
						meta.shader.backends.push_back(static_cast<EShaderBackend>(v.GetUint()));
				}
			}
			if (settings.HasMember("entryPoints") && settings["entryPoints"].IsArray())
			{
				meta.shader.entryPoints.clear();
				for (auto &v : settings["entryPoints"].GetArray())
				{
					if (v.IsString())
					{
						// Name-only form: stage inferred downstream.
						meta.shader.entryPoints.push_back(HMeta::ShaderEntryPoint{.name = v.GetString(), .stage = {}});
					}
					else if (v.IsObject())
					{
						HMeta::ShaderEntryPoint ep;
						if (v.HasMember("name") && v["name"].IsString())
							ep.name = v["name"].GetString();
						if (v.HasMember("stage") && v["stage"].IsString())
							ep.stage = v["stage"].GetString();
						if (!ep.name.empty())
							meta.shader.entryPoints.push_back(std::move(ep));
					}
				}
			}
			if (settings.HasMember("defines") && settings["defines"].IsArray())
			{
				meta.shader.defines.clear();
				for (auto &v : settings["defines"].GetArray())
				{
					if (v.IsString())
						meta.shader.defines.push_back(v.GetString());
				}
			}

			// Model settings
			if (settings.HasMember("importMaterials") && settings["importMaterials"].IsBool())
				meta.model.importMaterials = settings["importMaterials"].GetBool();
			if (settings.HasMember("generateLods") && settings["generateLods"].IsBool())
				meta.model.generateLods = settings["generateLods"].GetBool();
			if (settings.HasMember("scaleFactor") && settings["scaleFactor"].IsNumber())
				meta.model.scaleFactor = static_cast<float>(settings["scaleFactor"].GetDouble());
		}

		return meta;
	}

} // namespace Hush
