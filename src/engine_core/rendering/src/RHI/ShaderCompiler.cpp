/*! \file ShaderCompiler.cpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief Slang-based shader compiler implementation.
*/

#include "ShaderCompiler.hpp"
#include "Logger.hpp"

#include <slang.h>
#include <slang-com-ptr.h>

#include <cassert>
#include <fstream>
#include <sstream>

namespace Hush::Graphics
{
	static SlangCompileTarget ToSlangCompileTarget(EShaderTarget target)
	{
		switch (target)
		{
		case EShaderTarget::WGSL:
			return SLANG_WGSL;
		case EShaderTarget::SPIRV:
			return SLANG_SPIRV;
		case EShaderTarget::HLSL:
			return SLANG_HLSL;
		case EShaderTarget::DXIL:
			return SLANG_DXIL;
		default:
			return SLANG_WGSL;
		}
	}

	static void AppendDiagnostics(ISlangBlob *blob, std::string &outDiagnostics)
	{
		if (blob == nullptr)
		{
			return;
		}
		const auto *text = static_cast<const char *>(blob->getBufferPointer());
		if (text != nullptr && blob->getBufferSize() > 0)
		{
			outDiagnostics += text;
		}
	}

	static EBindingType FromSlangBindingType(slang::ParameterCategory category)
	{
		switch (category)
		{
		case slang::ParameterCategory::ConstantBuffer:
			return EBindingType::UniformBuffer;
		case slang::ParameterCategory::ShaderResource:
			return EBindingType::SampledTexture;
		case slang::ParameterCategory::UnorderedAccess:
			return EBindingType::StorageBuffer;
		case slang::ParameterCategory::SamplerState:
			return EBindingType::Sampler;
		case slang::ParameterCategory::Uniform:
			return EBindingType::UniformBuffer;
		default:
			return EBindingType::UniformBuffer;
		}
	}

	static EVertexFormat FromSlangTypeToVertexFormat(uint32_t scalarType, uint32_t rows, uint32_t columns)
	{
		// For vertex inputs, the column count typically represents the vector width.
		// Some reflection reports vectors as rows instead of columns.
		uint32_t componentCount = columns > 0 ? columns : 1;
		if (rows > 1 && columns <= 1)
		{
			componentCount = rows;
		}

		switch (static_cast<SlangScalarType>(scalarType))
		{
		case SLANG_SCALAR_TYPE_FLOAT32: {
			switch (componentCount)
			{
			case 1:
				return EVertexFormat::Float32;
			case 2:
				return EVertexFormat::Float32x2;
			case 3:
				return EVertexFormat::Float32x3;
			default:
				return EVertexFormat::Float32x4;
			}
		}
		case SLANG_SCALAR_TYPE_INT32: {
			switch (componentCount)
			{
			case 1:
				return EVertexFormat::Sint32;
			case 2:
				return EVertexFormat::Sint32x2;
			case 3:
				return EVertexFormat::Sint32x3;
			default:
				return EVertexFormat::Sint32x4;
			}
		}
		case SLANG_SCALAR_TYPE_UINT32: {
			switch (componentCount)
			{
			case 1:
				return EVertexFormat::Uint32;
			case 2:
				return EVertexFormat::Uint32x2;
			case 3:
				return EVertexFormat::Uint32x3;
			default:
				return EVertexFormat::Uint32x4;
			}
		}
		case SLANG_SCALAR_TYPE_FLOAT16: {
			if (componentCount <= 2)
			{
				return EVertexFormat::Float16x2;
			}
			return EVertexFormat::Float16x4;
		}
		default:
			return EVertexFormat::Float32x4;
		}
	}

	static void ReflectParameterBinding(slang::VariableLayoutReflection *param,
										const std::vector<CompiledShaderStage> &stages,
										std::vector<ReflectedBinding> &outBindings)
	{
		if (param == nullptr)
		{
			return;
		}

		slang::TypeLayoutReflection *typeLayout = param->getTypeLayout();
		if (typeLayout == nullptr)
		{
			return;
		}

		slang::TypeReflection::Kind kind = typeLayout->getType()->getKind();

		ReflectedBinding binding{};
		const char *paramName = param->getName();
		binding.name = (paramName != nullptr) ? paramName : "";

		// Determine set and binding from the Slang layout
		binding.set = static_cast<uint32_t>(param->getBindingSpace());
		binding.binding = static_cast<uint32_t>(param->getBindingIndex());

		// Determine binding type from the Slang type kind and category
		auto category = typeLayout->getParameterCategory();
		binding.type = FromSlangBindingType(category);

		// For buffer types, try to get the size
		if (binding.type == EBindingType::UniformBuffer || binding.type == EBindingType::StorageBuffer ||
			binding.type == EBindingType::ReadOnlyStorageBuffer)
		{
			binding.bufferSize = typeLayout->getSize();
		}

		// Stage visibility: derive from the compiled stages
		if (!stages.empty())
		{
			binding.stageFlags = EShaderStageFlags::None;
			for (const auto &stage : stages)
			{
				switch (stage.stage)
				{
				case EShaderStage::Vertex:
					binding.stageFlags |= EShaderStageFlags::Vertex;
					break;
				case EShaderStage::Fragment:
					binding.stageFlags |= EShaderStageFlags::Fragment;
					break;
				case EShaderStage::Compute:
					binding.stageFlags |= EShaderStageFlags::Compute;
					break;
				default:
					break;
				}
			}
		}
		else
		{
			binding.stageFlags = EShaderStageFlags::All;
		}

		// Refine binding type based on the Slang type kind
		if (kind == slang::TypeReflection::Kind::ConstantBuffer || kind == slang::TypeReflection::Kind::ParameterBlock)
		{
			binding.type = EBindingType::UniformBuffer;
			auto *elementTypeLayout = typeLayout->getElementTypeLayout();
			if (elementTypeLayout != nullptr)
			{
				binding.bufferSize = elementTypeLayout->getSize();
			}
		}
		else if (kind == slang::TypeReflection::Kind::Resource)
		{
			auto shape = typeLayout->getType()->getResourceShape();
			auto access = typeLayout->getType()->getResourceAccess();

			bool isTexture = (shape == SLANG_TEXTURE_2D || shape == SLANG_TEXTURE_3D || shape == SLANG_TEXTURE_CUBE ||
							  shape == SLANG_TEXTURE_1D || shape == SLANG_TEXTURE_2D_ARRAY);
			if (isTexture)
			{
				binding.type = (access == SLANG_RESOURCE_ACCESS_READ_WRITE) ? EBindingType::StorageTexture
																			: EBindingType::SampledTexture;
			}
			else if (shape == SLANG_STRUCTURED_BUFFER)
			{
				binding.type = (access == SLANG_RESOURCE_ACCESS_READ_WRITE) ? EBindingType::StorageBuffer
																			: EBindingType::ReadOnlyStorageBuffer;
			}
		}
		else if (kind == slang::TypeReflection::Kind::SamplerState)
		{
			binding.type = EBindingType::Sampler;
		}

		outBindings.push_back(binding);
	}

	static void ReflectVertexInputsFromStruct(slang::TypeLayoutReflection *inputType,
											  std::vector<ReflectedVertexInput> &outInputs)
	{
		auto fieldCount = static_cast<uint32_t>(inputType->getFieldCount());
		for (uint32_t fIdx = 0; fIdx < fieldCount; ++fIdx)
		{
			slang::VariableLayoutReflection *field = inputType->getFieldByIndex(fIdx);
			if (field == nullptr)
			{
				continue;
			}

			ReflectedVertexInput vertInput{};
			vertInput.location = static_cast<uint32_t>(field->getBindingIndex());
			const char *fieldName = field->getName();
			vertInput.semanticName = (fieldName != nullptr) ? fieldName : "";

			slang::TypeLayoutReflection *fieldType = field->getTypeLayout();
			if (fieldType != nullptr)
			{
				auto *scalarType = fieldType->getType();
				auto fieldRows = static_cast<uint32_t>(scalarType->getRowCount());
				auto fieldCols = static_cast<uint32_t>(scalarType->getColumnCount());
				auto scalar = scalarType->getScalarType();
				vertInput.format = FromSlangTypeToVertexFormat(static_cast<uint32_t>(scalar), fieldRows, fieldCols);
			}

			outInputs.push_back(vertInput);
		}
	}

	static void ReflectVertexInputs(slang::ProgramLayout *layout, std::vector<ReflectedVertexInput> &outInputs)
	{
		auto epCount = static_cast<uint32_t>(layout->getEntryPointCount());
		for (uint32_t epIdx = 0; epIdx < epCount; ++epIdx)
		{
			slang::EntryPointReflection *epReflection = layout->getEntryPointByIndex(epIdx);
			if (epReflection == nullptr)
			{
				continue;
			}

			if (epReflection->getStage() != SLANG_STAGE_VERTEX)
			{
				continue;
			}

			auto inputParamCount = static_cast<uint32_t>(epReflection->getParameterCount());
			for (uint32_t pIdx = 0; pIdx < inputParamCount; ++pIdx)
			{
				slang::VariableLayoutReflection *inputParam = epReflection->getParameterByIndex(pIdx);
				if (inputParam == nullptr)
				{
					continue;
				}

				auto paramCategory = inputParam->getCategory();
				if (paramCategory != slang::ParameterCategory::VaryingInput)
				{
					continue;
				}

				slang::TypeLayoutReflection *inputType = inputParam->getTypeLayout();
				if (inputType == nullptr)
				{
					continue;
				}

				auto inputKind = inputType->getType()->getKind();
				if (inputKind == slang::TypeReflection::Kind::Struct)
				{
					ReflectVertexInputsFromStruct(inputType, outInputs);
				}
				else
				{
					ReflectedVertexInput vertInput{};
					vertInput.location = static_cast<uint32_t>(inputParam->getBindingIndex());
					const char *inputName = inputParam->getName();
					vertInput.semanticName = (inputName != nullptr) ? inputName : "";

					auto *scalarType = inputType->getType();
					auto inputRows = static_cast<uint32_t>(scalarType->getRowCount());
					auto inputCols = static_cast<uint32_t>(scalarType->getColumnCount());
					auto scalar = scalarType->getScalarType();
					vertInput.format = FromSlangTypeToVertexFormat(static_cast<uint32_t>(scalar), inputRows, inputCols);

					outInputs.push_back(vertInput);
				}
			}

			// Only process the first vertex entry point
			break;
		}
	}

	ShaderCompiler::ShaderCompiler() = default;

	ShaderCompiler::~ShaderCompiler()
	{
		if (m_globalSession != nullptr)
		{
			m_globalSession->release();
			m_globalSession = nullptr;
		}
	}

	ShaderCompiler::ShaderCompiler(ShaderCompiler &&other) noexcept
		: m_globalSession(other.m_globalSession),
		  m_options(std::move(other.m_options)),
		  m_initialized(other.m_initialized),
		  m_cache(std::move(other.m_cache))
	{
		other.m_globalSession = nullptr;
		other.m_initialized = false;
	}

	ShaderCompiler &ShaderCompiler::operator=(ShaderCompiler &&other) noexcept
	{
		if (this != &other)
		{
			if (m_globalSession != nullptr)
			{
				m_globalSession->release();
			}
			m_globalSession = other.m_globalSession;
			m_options = std::move(other.m_options);
			m_initialized = other.m_initialized;
			m_cache = std::move(other.m_cache);

			other.m_globalSession = nullptr;
			other.m_initialized = false;
		}
		return *this;
	}

	bool ShaderCompiler::Initialize(const ShaderCompilerOptions &options)
	{
		// Clean up previous session if reinitializing
		if (m_globalSession != nullptr)
		{
			m_globalSession->release();
			m_globalSession = nullptr;
			m_initialized = false;
			m_cache.clear();
		}

		m_options = options;

		// Create the Slang global session
		SlangResult result = slang::createGlobalSession(&m_globalSession);
		if (SLANG_FAILED(result) || m_globalSession == nullptr)
		{
			Hush::LogFormat(ELogLevel::Error, "ShaderCompiler: Failed to create Slang global session (result: 0x%08X)",
							static_cast<uint32_t>(result));
			return false;
		}

		m_initialized = true;
		Hush::LogInfo("ShaderCompiler: Initialized successfully");
		return true;
	}

	bool ShaderCompiler::IsInitialized() const
	{
		return m_initialized;
	}

	EShaderTarget ShaderCompiler::GetTarget() const
	{
		return m_options.target;
	}

	ShaderCompilationResult ShaderCompiler::CompileFromSource(std::string_view source, std::string_view sourceName,
															  const std::vector<ShaderEntryPointRequest> &entryPoints)
	{
		if (!m_initialized)
		{
			ShaderCompilationResult failResult;
			failResult.success = false;
			failResult.diagnostics = "ShaderCompiler is not initialized.";
			return failResult;
		}

		std::string nameStr(sourceName);

		// Check cache
		std::string cacheKey = BuildCacheKey(nameStr.c_str(), entryPoints);
		auto cacheIt = m_cache.find(cacheKey);
		if (cacheIt != m_cache.end())
		{
			return cacheIt->second;
		}

		auto result = CompileInternal(nameStr.c_str(), source.data(), source.size(), entryPoints);

		// Cache the result
		m_cache[cacheKey] = result;
		return result;
	}

	void ShaderCompiler::ClearCache()
	{
		m_cache.clear();
	}

	size_t ShaderCompiler::GetCacheSize() const
	{
		return m_cache.size();
	}

	EShaderTarget ShaderCompiler::GetTargetForAPI(EGraphicsAPI api)
	{
		switch (api)
		{
		case EGraphicsAPI::WebGPU:
			return EShaderTarget::WGSL;
		case EGraphicsAPI::Vulkan:
			return EShaderTarget::SPIRV;
		case EGraphicsAPI::D3D12:
			return EShaderTarget::DXIL;
		case EGraphicsAPI::Metal:
			return EShaderTarget::SPIRV;
		default:
			return EShaderTarget::WGSL;
		}
	}

	const char *ShaderCompiler::GetSlangProfileForStage(EShaderStage stage, EShaderTarget target)
	{
		switch (target)
		{
		case EShaderTarget::HLSL:
		case EShaderTarget::DXIL: {
			switch (stage)
			{
			case EShaderStage::Vertex:
				return "vs_6_0";
			case EShaderStage::Fragment:
				return "ps_6_0";
			case EShaderStage::Compute:
				return "cs_6_0";
			default:
				return "vs_6_0";
			}
		}
		case EShaderTarget::SPIRV: {
			switch (stage)
			{
			case EShaderStage::Vertex:
				return "glsl_vertex";
			case EShaderStage::Fragment:
				return "glsl_fragment";
			case EShaderStage::Compute:
				return "glsl_compute";
			default:
				return "glsl_vertex";
			}
		}
		case EShaderTarget::WGSL:
		default:
			// Slang handles WGSL profiles internally
			return "";
		}
	}

	ShaderCompilationResult ShaderCompiler::CompileInternal(const char *moduleNameOrPath, const char *source,
															size_t sourceLength,
															const std::vector<ShaderEntryPointRequest> &entryPoints)
	{
		ShaderCompilationResult result;
		result.success = false;

		slang::SessionDesc sessionDesc = {};

		// Set up the target
		slang::TargetDesc targetDesc = {};
		targetDesc.format = ToSlangCompileTarget(m_options.target);

		// Slang sets optimization at session level via defaultMatrixLayoutMode
		// but the optimization level is a property of the target
		targetDesc.flags = 0;
		if (m_options.generateDebugInfo)
		{
			targetDesc.flags = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
		}

		sessionDesc.targets = &targetDesc;
		sessionDesc.targetCount = 1;

		// Set the default optimization level on the session
		sessionDesc.compilerOptionEntryCount = 0;

		// Set up search paths
		std::vector<const char *> searchPathPtrs;
		searchPathPtrs.reserve(m_options.includePaths.size());
		for (const auto &path : m_options.includePaths)
		{
			searchPathPtrs.push_back(path.c_str());
		}
		sessionDesc.searchPaths = searchPathPtrs.data();
		sessionDesc.searchPathCount = static_cast<SlangInt>(searchPathPtrs.size());

		// Preprocessor defines
		std::vector<slang::PreprocessorMacroDesc> macros;
		macros.reserve(m_options.defines.size());
		for (const auto &[name, value] : m_options.defines)
		{
			slang::PreprocessorMacroDesc macro = {};
			macro.name = name.c_str();
			macro.value = value.empty() ? "1" : value.c_str();
			macros.push_back(macro);
		}
		sessionDesc.preprocessorMacros = macros.data();
		sessionDesc.preprocessorMacroCount = static_cast<SlangInt>(macros.size());

		// Default matrix layout
		if (m_options.matrixLayout == 1)
		{
			sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
		}
		else
		{
			sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_ROW_MAJOR;
		}

		Slang::ComPtr<slang::ISession> session;
		SlangResult sr = m_globalSession->createSession(sessionDesc, session.writeRef());
		if (SLANG_FAILED(sr) || session.get() == nullptr)
		{
			result.diagnostics = "Failed to create Slang session.";
			return result;
		}

		Slang::ComPtr<ISlangBlob> diagnosticsBlob;
		slang::IModule *module = nullptr;

		if (source != nullptr && sourceLength > 0)
		{
			// Load from source string
			module = session->loadModuleFromSourceString(moduleNameOrPath,
														 moduleNameOrPath, // virtual path for diagnostics
														 source, diagnosticsBlob.writeRef());
		}
		else
		{
			module = session->loadModule(moduleNameOrPath, diagnosticsBlob.writeRef());
		}

		AppendDiagnostics(diagnosticsBlob.get(), result.diagnostics);

		if (module == nullptr)
		{
			if (result.diagnostics.empty())
			{
				result.diagnostics = "Failed to load Slang module: ";
				result.diagnostics += moduleNameOrPath;
			}
			return result;
		}

		std::vector<Slang::ComPtr<slang::IEntryPoint>> slangEntryPoints;
		slangEntryPoints.reserve(entryPoints.size());

		for (const auto &ep : entryPoints)
		{
			Slang::ComPtr<slang::IEntryPoint> entryPoint;

			sr = module->findEntryPointByName(ep.entryPointName.c_str(), entryPoint.writeRef());
			if (SLANG_FAILED(sr) || entryPoint.get() == nullptr)
			{
				// Try finding with explicit stage
				auto stage = static_cast<SlangStage>(ToSlangStage(ep.stage));
				diagnosticsBlob = nullptr;

				sr = module->findAndCheckEntryPoint(ep.entryPointName.c_str(), stage, entryPoint.writeRef(),
													diagnosticsBlob.writeRef());

				AppendDiagnostics(diagnosticsBlob.get(), result.diagnostics);

				if (SLANG_FAILED(sr) || entryPoint.get() == nullptr)
				{
					result.diagnostics += "\nFailed to find entry point: " + ep.entryPointName;
					return result;
				}
			}

			slangEntryPoints.push_back(entryPoint);
		}

		std::vector<slang::IComponentType *> components;
		components.reserve(1 + slangEntryPoints.size());
		components.push_back(module);
		for (auto &ep : slangEntryPoints)
		{
			components.push_back(ep.get());
		}

		Slang::ComPtr<slang::IComponentType> composedProgram;
		diagnosticsBlob = nullptr;

		sr = session->createCompositeComponentType(components.data(), static_cast<SlangInt>(components.size()),
												   composedProgram.writeRef(), diagnosticsBlob.writeRef());

		AppendDiagnostics(diagnosticsBlob.get(), result.diagnostics);

		if (SLANG_FAILED(sr) || composedProgram.get() == nullptr)
		{
			result.diagnostics += "\nFailed to compose Slang program.";
			return result;
		}

		Slang::ComPtr<slang::IComponentType> linkedProgram;
		diagnosticsBlob = nullptr;

		sr = composedProgram->link(linkedProgram.writeRef(), diagnosticsBlob.writeRef());

		AppendDiagnostics(diagnosticsBlob.get(), result.diagnostics);

		if (SLANG_FAILED(sr) || linkedProgram.get() == nullptr)
		{
			result.diagnostics += "\nFailed to link Slang program.";
			return result;
		}

		result.stages.resize(entryPoints.size());
		bool allStagesOk = true;

		for (size_t i = 0; i < entryPoints.size(); ++i)
		{
			result.stages[i].stage = entryPoints[i].stage;
			result.stages[i].entryPoint = entryPoints[i].entryPointName;

			if (!ExtractEntryPointCode(linkedProgram.get(), static_cast<uint32_t>(i), entryPoints[i], result.stages[i],
									   result.diagnostics))
			{
				allStagesOk = false;
			}
		}

		if (!allStagesOk)
		{
			return result;
		}

		ExtractReflection(linkedProgram.get(), result);

		result.success = true;
		return result;
	}

	bool ShaderCompiler::ExtractEntryPointCode(slang::IComponentType *program, uint32_t entryPointIndex,
											   const ShaderEntryPointRequest &request, CompiledShaderStage &outStage,
											   std::string &outDiagnostics) const
	{
		Slang::ComPtr<ISlangBlob> codeBlob;
		Slang::ComPtr<ISlangBlob> diagnosticsBlob;

		SlangResult sr = program->getEntryPointCode(static_cast<SlangInt>(entryPointIndex),
													0, // target index (we only have one target)
													codeBlob.writeRef(), diagnosticsBlob.writeRef());

		AppendDiagnostics(diagnosticsBlob.get(), outDiagnostics);

		if (SLANG_FAILED(sr) || codeBlob.get() == nullptr)
		{
			outDiagnostics += "\nFailed to get compiled code for entry point: " + request.entryPointName;
			return false;
		}

		// Populate the ShaderModuleDescriptor
		outStage.moduleDesc.stage = request.stage;
		outStage.moduleDesc.entryPoint = request.entryPointName;
		outStage.moduleDesc.debugName = request.entryPointName;

		const void *codeData = codeBlob->getBufferPointer();
		size_t codeSize = codeBlob->getBufferSize();

		if (m_options.target == EShaderTarget::WGSL || m_options.target == EShaderTarget::HLSL)
		{
			// Text-based output
			outStage.moduleDesc.bytecode.content =
				ShaderBytecode::TextContent{std::string(static_cast<const char *>(codeData), codeSize)};
		}
		else
		{
			// Binary output (SPIR-V, DXIL)
			const auto *bytePtr = static_cast<const uint8_t *>(codeData);
			outStage.moduleDesc.bytecode.content =
				ShaderBytecode::BinaryContent{.data = std::vector<uint8_t>(bytePtr, bytePtr + codeSize)};
		}

		return true;
	}

	void ShaderCompiler::ExtractReflection(slang::IComponentType *program, ShaderCompilationResult &outResult)
	{
		slang::ProgramLayout *layout = program->getLayout(0); // target index 0
		if (layout == nullptr)
		{
			outResult.diagnostics += "\nWarning: Could not retrieve program layout for reflection.";
			return;
		}

		auto parameterCount = static_cast<uint32_t>(layout->getParameterCount());
		for (uint32_t paramIdx = 0; paramIdx < parameterCount; ++paramIdx)
		{
			slang::VariableLayoutReflection *param = layout->getParameterByIndex(paramIdx);
			ReflectParameterBinding(param, outResult.stages, outResult.bindings);
		}

		ReflectVertexInputs(layout, outResult.vertexInputs);
	}

	std::string ShaderCompiler::BuildCacheKey(const char *moduleNameOrPath,
											  const std::vector<ShaderEntryPointRequest> &entryPoints) const
	{
		std::string key;
		key += moduleNameOrPath;
		key += "|target=";
		key += std::to_string(static_cast<uint32_t>(m_options.target));
		key += "|opt=";
		key += std::to_string(m_options.optimizationLevel);

		for (const auto &ep : entryPoints)
		{
			key += "|ep=";
			key += ep.entryPointName;
			key += ":";
			key += std::to_string(static_cast<uint32_t>(ep.stage));
		}

		for (const auto &[name, value] : m_options.defines)
		{
			key += "|D=";
			key += name;
			key += "=";
			key += value;
		}

		return key;
	}

	int32_t ShaderCompiler::ToSlangStage(EShaderStage stage)
	{
		switch (stage)
		{
		case EShaderStage::Vertex:
			return SLANG_STAGE_VERTEX;
		case EShaderStage::Fragment:
			return SLANG_STAGE_FRAGMENT;
		case EShaderStage::Compute:
			return SLANG_STAGE_COMPUTE;
		default:
			return SLANG_STAGE_VERTEX;
		}
	}

	EBindingType ShaderCompiler::FromSlangBindingType(int32_t slangCategory)
	{
		return Hush::Graphics::FromSlangBindingType(static_cast<slang::ParameterCategory>(slangCategory));
	}

	EVertexFormat ShaderCompiler::FromSlangTypeToVertexFormat(uint32_t scalarType, uint32_t rows, uint32_t columns)
	{
		return Hush::Graphics::FromSlangTypeToVertexFormat(scalarType, rows, columns);
	}
} // namespace Hush::Graphics
