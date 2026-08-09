#include "ShaderCooker.hpp"
#include "HShader.hpp"
#include "crypto/Hashing.hpp"
#include "Logger.hpp"

#pragma warning(push)
#pragma warning(disable : 4702) // unreachable code in IShaderModule.hpp IsValid()
#include "RHI/ShaderCompiler.hpp"
#pragma warning(pop)
#include <algorithm>
#include <cstring>
#include <zstd.h>

namespace Hush
{

	ShaderCooker::ShaderCooker(Graphics::ShaderCompiler *compiler)
		: m_compiler(compiler)
	{
	}

	ShaderCooker::~ShaderCooker() = default;

	std::span<const EFileExtension> ShaderCooker::SupportedExtensions() const
	{
		return EXTENSIONS;
	}

	bool ShaderCooker::CanCook(const FileInfo &info) const
	{
		return info.extension == EFileExtension::SLANG;
	}

	HMeta ShaderCooker::DefaultMeta(EFileExtension ext, std::string_view srcVPath) const
	{
		HMeta meta;
		meta.id = Hashing::Fnv1a(srcVPath);
		meta.assetType = "shader";
		meta.outputFormat = EAssetFormat::Shader;
		meta.compression = ECompressionFormat::Zstd;
		meta.shader.backends = {EShaderBackend::WebGPU_WGSL};
		// Default entry points: look for [shader("...")] attributes in source
		// For now, just leave empty and detect during Cook
		(void)ext;
		return meta;
	}

	/// Simple entry point detection: search for [shader("...")] attributes in source.
	static void DetectEntryPoints(std::string_view source,
								  std::vector<Graphics::ShaderEntryPointRequest> &outEntryPoints)
	{
		// Search for [shader("xxx")] patterns using simple substring search
		static constexpr std::string_view kAttrPrefix = "[shader(\"";
		static constexpr std::string_view kAttrSuffix = "\")]";

		size_t pos = 0;
		while (true)
		{
			pos = source.find(kAttrPrefix, pos);
			if (pos == std::string_view::npos)
				break;

			pos += kAttrPrefix.size();
			size_t end = source.find(kAttrSuffix, pos);
			if (end == std::string_view::npos)
				break;

			std::string_view stageName = source.substr(pos, end - pos);
			pos = end + kAttrSuffix.size();

			// Map stage name
			Graphics::EShaderStage stage = Graphics::EShaderStage::Vertex;
			if (stageName == "fragment" || stageName == "pixel")
				stage = Graphics::EShaderStage::Fragment;
			else if (stageName == "compute")
				stage = Graphics::EShaderStage::Compute;

			// Look for the next function name after the attribute
			// Simple heuristic: find " fnName(" pattern
			size_t fnStart = source.find_first_not_of(" \t\r\n", pos);
			if (fnStart != std::string_view::npos)
			{
				size_t fnEnd = source.find('(', fnStart);
				if (fnEnd != std::string_view::npos)
				{
					// Skip return type by finding the last identifier before '('
					size_t nameStart = source.find_last_of(" \t\r\n", fnEnd);
					if (nameStart != std::string_view::npos && nameStart > pos)
						nameStart++;
					else
						nameStart = fnStart;

					std::string funcName(source.substr(nameStart, fnEnd - nameStart));
					outEntryPoints.push_back({.stage = stage, .entryPointName = funcName});
					continue;
				}
			}

			// Fallback: just use "main"
			outEntryPoints.push_back({.stage = stage, .entryPointName = "main"});
		}

		if (outEntryPoints.empty())
		{
			// No [shader(...)] attributes found; add defaults
			outEntryPoints.push_back({.stage = Graphics::EShaderStage::Vertex, .entryPointName = "main"});
			outEntryPoints.push_back({.stage = Graphics::EShaderStage::Fragment, .entryPointName = "main"});
		}
	}

	Result<ShaderCooker::CookResult, ECookError> ShaderCooker::Cook(std::span<const std::byte> input, const HMeta &meta,
																	const CookContext &ctx)
	{
		// Slang's loadModuleFromSourceString takes a null-terminated C string (no length
		// arg), so back the view with a std::string whose data() is guaranteed NUL-terminated.
		// A view straight over the input span is not NUL-terminated and makes Slang read past
		// the buffer (garbage parse + out-of-bounds crash).
		std::string sourceStr(reinterpret_cast<const char *>(input.data()), input.size());
		std::string_view source(sourceStr);

		// Build entry point requests
		std::vector<Graphics::ShaderEntryPointRequest> entryPoints;

		if (!meta.shader.entryPoints.empty())
		{
			// Use entry points from HMeta
			for (const auto &ep : meta.shader.entryPoints)
			{
				Graphics::EShaderStage stage = Graphics::EShaderStage::Vertex;

				std::string lower = ep.stage.empty() ? ep.name : ep.stage;
				std::transform(lower.begin(), lower.end(), lower.begin(),
							   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

				if (!ep.stage.empty())
				{
					// Explicit stage from meta — unknown values are a config error.
					if (lower == "vertex")
					{
						stage = Graphics::EShaderStage::Vertex;
					}
					else if (lower == "fragment" || lower == "pixel")
					{
						stage = Graphics::EShaderStage::Fragment;
					}
					else if (lower == "compute")
					{
						stage = Graphics::EShaderStage::Compute;
					}
					else
					{
						LogFormat(ELogLevel::Error, "ShaderCooker: unknown stage '{}' for entry point '{}' in {}",
								  ep.stage, ep.name, ctx.sourceVPath);
						return ECookError::InvalidMeta;
					}
				}
				else
				{
					// Infer stage from common naming conventions
					if (lower.find("fragment") != std::string::npos || lower.find("pixel") != std::string::npos)
					{
						stage = Graphics::EShaderStage::Fragment;
					}
					else if (lower.find("compute") != std::string::npos)
					{
						stage = Graphics::EShaderStage::Compute;
					}
				}
				entryPoints.push_back({.stage = stage, .entryPointName = ep.name});
			}
		}
		else
		{
			// Auto-detect from source
			DetectEntryPoints(source, entryPoints);
		}

		if (entryPoints.empty())
		{
			LogFormat(ELogLevel::Error, "ShaderCooker: no entry points for {}", ctx.sourceVPath);
			return ECookError::InvalidMeta;
		}

		// Thread-safe compilation
		Graphics::ShaderCompilationResult compileResult;

		{
			std::lock_guard<std::mutex> lock(m_mutex);

			// Use the borrowed compiler, or a persistent owned one that is created once and
			// reused across cooks (so Slang init + the compile cache are shared, not per-cook).
			Graphics::ShaderCompiler *compiler = m_compiler;

			if (compiler == nullptr)
			{
				if (m_ownedCompiler == nullptr)
				{
					m_ownedCompiler = std::make_unique<Graphics::ShaderCompiler>();
				}
				compiler = m_ownedCompiler.get();
			}

			if (!compiler->IsInitialized())
			{
				Graphics::ShaderCompilerOptions opts;
				opts.target = Graphics::EShaderTarget::WGSL;
				opts.optimizationLevel = 1;
				if (!compiler->Initialize(opts))
				{
					LogFormat(ELogLevel::Error, "ShaderCooker: Slang initialization failed for {}", ctx.sourceVPath);
					return ECookError::Unknown;
				}
			}

			compileResult =
				compiler->CompileFromSource(NullTerminatedStringView(sourceStr), ctx.sourceVPath, entryPoints);
		}

		if (!compileResult.success)
		{
			LogFormat(ELogLevel::Error, "ShaderCooker: compilation failed for {}: {}", ctx.sourceVPath,
					  compileResult.diagnostics);
			return ECookError::DecodeFailed;
		}

		// Build HShader
		HShader shader;
		shader.header.magic = HSHADER_MAGIC;
		shader.header.version = HSHADER_VERSION;

		// Collect WGSL backend data, recording each stage's code range so the
		// serialized HShader can recover per-stage modules.
		HShader::BackendData backendData;

		for (const auto &stage : compileResult.stages)
		{
			// Extract WGSL text
			const auto &bc = stage.moduleDesc.bytecode;
			if (!bc.IsValid())
			{
				continue;
			}

			std::string wgslCode;
			if (auto *text = std::get_if<Graphics::ShaderBytecode::TextContent>(&bc.content))
			{
				wgslCode = text->sourceText;
			}
			else if (auto *bin = std::get_if<Graphics::ShaderBytecode::BinaryContent>(&bc.content))
			{
				wgslCode.assign(reinterpret_cast<const char *>(bin->data.data()), bin->data.size());
			}

			if (wgslCode.empty())
			{
				continue;
			}

			HShader::StageData stageData;
			stageData.stage = static_cast<uint32_t>(stage.stage);
			stageData.entryName = stage.entryPoint;
			stageData.codeOffset = backendData.bytecode.size();
			stageData.codeSize = wgslCode.size();
			backendData.stages.push_back(std::move(stageData));

			backendData.bytecode.insert(backendData.bytecode.end(),
										reinterpret_cast<const std::byte *>(wgslCode.data()),
										reinterpret_cast<const std::byte *>(wgslCode.data() + wgslCode.size()));
		}

		if (backendData.stages.empty())
		{
			LogFormat(ELogLevel::Error, "ShaderCooker: no usable bytecode for {}", ctx.sourceVPath);
			return ECookError::DecodeFailed;
		}

		BackendEntry be{};
		be.backendType = EShaderBackend::WebGPU_WGSL;
		be.stageCount = static_cast<uint32_t>(backendData.stages.size());

		shader.backends.push_back(be);
		shader.backendData.push_back(std::move(backendData));

		// Serialize HShader
		std::vector<std::byte> hshaderBlob;
		HShader::Write(hshaderBlob, shader);

		CookResult result;
		result.payload = std::move(hshaderBlob);
		result.format = EAssetFormat::Shader;
		return result;
	}

} // namespace Hush
