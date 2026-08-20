/*! \file ShaderCompiler.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Slang-based shader compiler that produces backend-specific bytecode
		   and reflection data.
*/
#pragma once

#include "BitwiseUtils.hpp"
#include "GraphicsTypes.hpp"
#include "IBindGroup.hpp"
#include "IShaderModule.hpp"
#include "PipelineDescriptor.hpp"

#include <NullTerminatedStringView.hpp>
#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

// Forward-declare Slang COM types used in the private interface.
// Full definitions are only needed in the .cpp file.
namespace slang
{
	struct IGlobalSession;
	struct IComponentType;
} // namespace slang

namespace Hush::Graphics
{
	/// @brief The target intermediate representation / language that the
	///        compiler should emit.
	enum class EShaderTarget : uint32_t
	{
		/// @brief WebGPU Shading Language (text)
		WGSL = 0,

		/// @brief SPIR-V bytecode (Vulkan)
		SPIRV,

		/// @brief HLSL source text (for further compilation to DXIL, or for
		///        debugging)
		HLSL,

		/// @brief DirectX Intermediate Language bytecode (D3D12)
		DXIL,
	};

	/// @brief Describes a single entry point to compile.
	struct ShaderEntryPointRequest
	{
		/// @brief The shader stage this entry point targets
		EShaderStage stage = EShaderStage::Vertex;

		/// @brief The name of the entry point function in the Slang source.
		/// For example: "vertexMain", "fragmentMain", "computeMain".
		std::string entryPointName = "main";
	};

	/// @brief Data type of the reflected binding, useful for inspector serialization
	/// @details These flags can be encoded in a way that tells you what the composed type is scalar + (vector / array
	/// dimensions). i.e:
	///    - (Float32 | Vec4 | AsColor) == RGBA8 color binding
	//     - (Float64 | Vec3) == Raw vec3 double precision vector, maybe for positions
	enum class EBindingDataTypeFlags : uint32_t
	{
		Undefined = 0,
		Int32 = 1,
		UInt32 = 2,
		Int64 = 4,
		UInt64 = 8,
		Float32 = 16,
		Float64 = 32,
		Vec2 = 64,
		Vec3 = 128,
		Vec4 = 256,
		Mat3 = 512,
		AsColor = 1024,
		// Should not be exposed in any user-facing API by default
		IsPrivate = 2048
	};

	HUSH_GENERATE_FLAGS(EBindingDataTypeFlags, uint32_t);

	/// @brief A single reflected resource binding extracted from compiled shaders.
	///
	/// The compiler fills these from Slang's reflection API so that callers can
	/// automatically build BindGroupLayoutDescriptors without manually
	/// duplicating the shader's binding declarations.
	struct ReflectedBinding
	{
		/// @brief The set / group index this binding belongs to.
		uint32_t set = 0;

		/// @brief The binding index within the set / group.
		uint32_t binding = 0;

		/// @brief The kind of resource
		EBindingType type = EBindingType::UniformBuffer;

		/// @brief Which shader stages reference this binding
		EShaderStageFlags stageFlags = EShaderStageFlags::None;

		/// @brief Debug name (from the shader source)
		std::string name;

		/// @brief For buffer bindings: minimum required size (0 = unknown)
		uint64_t bufferSize = 0;
		/// @brief The offset of the initial buffer pointer (mostly to handle struct fields of one giant buffer)
		uint64_t bufferOffset = 0;

		/// @brief True if this is a per-member sub-entry of a ConstantBuffer struct.
		/// When true, this entry represents a single field within a constant buffer
		/// and should be used for property mapping (not for layout building).
		bool isMember = false;

		EBindingDataTypeFlags dataType;
	};

	/// @brief Reflected vertex input attribute extracted from the vertex shader.
	struct ReflectedVertexInput
	{
		/// @brief Shader location index
		uint32_t location = 0;

		/// @brief Semantic name (if available from HLSL/Slang semantics)
		std::string semanticName;

		/// @brief Format of the attribute
		EVertexFormat format = EVertexFormat::Float32x4;
	};

	/// @brief Per-stage compilation output.
	struct CompiledShaderStage
	{
		/// @brief The stage this output corresponds to
		EShaderStage stage = EShaderStage::Vertex;

		/// @brief The entry point name
		std::string entryPoint;

		/// @brief A ready-to-use ShaderModuleDescriptor that can be passed
		///        directly to IGraphicsDevice::CreateShaderModule().
		ShaderModuleDescriptor moduleDesc;
	};

	/// @brief Full result of a shader compilation request.
	struct ShaderCompilationResult
	{
		/// @brief Whether compilation succeeded for all requested entry points.
		bool success = false;

		/// @brief Human-readable error / warning messages from the compiler.
		std::string diagnostics;

		/// @brief Per-stage compilation outputs (one per ShaderEntryPointRequest).
		std::vector<CompiledShaderStage> stages;

		/// @brief Reflected resource bindings (union across all stages).
		std::vector<ReflectedBinding> bindings;

		/// @brief Reflected vertex inputs (from the vertex stage, if present).
		std::vector<ReflectedVertexInput> vertexInputs;

		/// @brief Helper: find a compiled stage by type.
		/// @return Pointer to the stage, or nullptr if not found.
		[[nodiscard]]
		const CompiledShaderStage *FindStage(EShaderStage stage) const
		{
			for (const auto &s : stages)
			{
				if (s.stage == stage)
				{
					return &s;
				}
			}
			return nullptr;
		}

		/// @brief Helper: build BindGroupLayoutDescriptors from reflected bindings.
		///
		/// Groups the reflected bindings by set index and produces one layout
		/// descriptor per set.  The returned vector is indexed by set number
		/// (gaps are filled with empty descriptors).
		[[nodiscard]]
		std::vector<BindGroupLayoutDescriptor> BuildBindGroupLayoutDescriptors() const
		{
			// Determine the maximum set index used.
			uint32_t maxSet = 0;
			for (const auto &b : bindings)
			{
				maxSet = std::max(b.set, maxSet);
			}

			std::vector<BindGroupLayoutDescriptor> layouts(maxSet + 1);

			for (const auto &b : bindings)
			{
				// Per-member sub-entries do not represent distinct layout bindings.
				if (b.isMember)
				{
					continue;
				}

				BindGroupLayoutEntry entry{};
				entry.binding = b.binding;
				entry.type = b.type;
				entry.stageFlags = b.stageFlags;
				entry.minBufferBindingSize = b.bufferSize;
				layouts[b.set].entries.push_back(entry);
			}

			return layouts;
		}
	};

	/// @brief Compiler-wide options.
	struct ShaderCompilerOptions
	{
		/// @brief The backend target to compile for.
		EShaderTarget target = EShaderTarget::WGSL;

		/// @brief Optimisation level (0 = none, 1 = default, 2 = performance,
		///        3 = size).  Maps to Slang's optimisation settings.
		uint32_t optimizationLevel = 1;

		/// @brief Generate debug information in the output.
		bool generateDebugInfo = false;

		/// @brief Additional include search paths for #include / import.
		std::vector<std::string> includePaths;

		/// @brief Preprocessor macro definitions (name, value pairs).
		///        An empty value means a define with no value (like -DFOO).
		std::vector<std::pair<std::string, std::string>> defines;

		/// @brief The matrix layout to use for uniform buffers.
		/// 0 = row-major (Slang default), 1 = column-major.
		uint32_t matrixLayout = 0;
	};

	/// @brief Slang-based shader compiler.
	///
	/// Compiles .slang shader source files (or raw strings) to the target
	/// backend format (WGSL, SPIR-V, HLSL, DXIL) and extracts reflection data.
	///
	/// The compiler owns a Slang global session and per-compilation sessions.
	/// It caches compiled results keyed by (source identity + entry points +
	/// target) so that repeated compilations of the same shader are free.
	class ShaderCompiler
	{
	public:
		ShaderCompiler();
		~ShaderCompiler();

		ShaderCompiler(const ShaderCompiler &) = delete;
		ShaderCompiler &operator=(const ShaderCompiler &) = delete;

		ShaderCompiler(ShaderCompiler &&other) noexcept;
		ShaderCompiler &operator=(ShaderCompiler &&other) noexcept;

		/// @brief Initialize the Slang global session with the given options.
		///
		/// Must be called once before any compilation.  Can be called again to
		/// reinitialise with different options (clears the cache).
		///
		/// @param options Compiler options (target, optimisation, etc.).
		/// @return True on success, false if Slang initialisation failed.
		bool Initialize(const ShaderCompilerOptions &options = {});

		/// @brief Check if the compiler has been initialized successfully.
		[[nodiscard]]
		bool IsInitialized() const;

		/// @brief Get the current target backend.
		[[nodiscard]]
		EShaderTarget GetTarget() const;

		/// @brief Compile a raw Slang source string with the given entry points.
		///
		/// @param source     Slang source code.
		/// @param sourceName Virtual file name for diagnostics (e.g. "inline.slang").
		/// @param entryPoints List of entry points to compile.
		/// @return Compilation result.
		[[nodiscard]]
		ShaderCompilationResult CompileFromSource(NullTerminatedStringView source, NullTerminatedStringView sourceName,
												  const std::vector<ShaderEntryPointRequest> &entryPoints);

		/// @brief Clear all cached compilation results.
		///
		/// Call this when shaders are modified at runtime (hot-reload) to force
		/// recompilation on the next request.
		void ClearCache();

		/// @brief Get the number of cached compilation results.
		[[nodiscard]]
		size_t GetCacheSize() const;

		/// @brief Helper to select the appropriate EShaderTarget for a given
		///        graphics API backend.
		[[nodiscard]]
		static EShaderTarget GetTargetForAPI(EGraphicsAPI api);

		/// @brief Helper: given an EShaderStage, return the Slang profile name
		///        string (e.g. "vs_6_0", "ps_6_0", "cs_6_0" for HLSL-family
		///        targets, or the WGSL / GLSL equivalents).
		[[nodiscard]]
		static const char *GetSlangProfileForStage(EShaderStage stage, EShaderTarget target);

	private:
		/// @brief Shared compilation logic used by both CompileFromFile and
		///        CompileFromSource.
		ShaderCompilationResult CompileInternal(NullTerminatedStringView moduleNameOrPath,
												NullTerminatedStringView source, // empty when compiling from file
												const std::vector<ShaderEntryPointRequest> &entryPoints);

		/// @brief Extract reflection data from a linked Slang program.
		void ExtractReflection(slang::IComponentType *program, ShaderCompilationResult &outResult);

		/// @brief Read the compiled code blob for a single entry point and
		///        populate the CompiledShaderStage.
		bool ExtractEntryPointCode(slang::IComponentType *program, uint32_t entryPointIndex,
								   const ShaderEntryPointRequest &request, CompiledShaderStage &outStage,
								   std::string &outDiagnostics) const;

		/// @brief Build a cache key from the compilation inputs.
		[[nodiscard]]
		std::string BuildCacheKey(NullTerminatedStringView moduleNameOrPath,
								  const std::vector<ShaderEntryPointRequest> &entryPoints) const;

		/// @brief Map an EShaderStage to the Slang SlangStage enum value.
		[[nodiscard]]
		static int32_t ToSlangStage(EShaderStage stage);

		/// @brief Map Slang's reflection binding type to our EBindingType.
		[[nodiscard]]
		static EBindingType FromSlangBindingType(int32_t slangCategory);

		/// @brief Map Slang's reflected scalar/vector type to our EVertexFormat.
		[[nodiscard]]
		static EVertexFormat FromSlangTypeToVertexFormat(uint32_t scalarType, uint32_t rows, uint32_t columns);

		/// @brief The Slang global session (manages the Slang runtime).
		slang::IGlobalSession *m_globalSession = nullptr;

		/// @brief Current compiler options.
		ShaderCompilerOptions m_options;

		/// @brief Whether Initialize() has been called successfully.
		bool m_initialized = false;

		/// @brief Compilation cache (key -> result).
		std::unordered_map<std::string, ShaderCompilationResult> m_cache;
	};

} // namespace Hush::Graphics
