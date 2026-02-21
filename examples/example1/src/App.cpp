#include "HushEngine.hpp"
#include "IApplication.hpp"
#include "ISystem.hpp"
#include "RHI/GraphicsResources.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "RHI/IBindGroup.hpp"
#include "RHI/IGraphicsDevice.hpp"

#include "RHI/IShaderModule.hpp"
#include "RHI/PipelineDescriptor.hpp"
#include "RHI/ShaderCompiler.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Systems/RenderGraphSystem.hpp"
#include "Scene.hpp"
#include "WindowRenderer.hpp"
#include <cstring>
#include <iostream>
#include <memory>

// NOLINTBEGIN(*-avoid-c-arrays)
static constexpr const char *TRIANGLE_SHADER_SOURCE = R"(
struct Uniforms
{
    float4x4 mvp;
    float4   tintColor;
    float    time;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Uniforms> uniforms;

struct VertexOutput
{
    float4 position : SV_Position;
    float3 color    : COLOR0;
    float2 uv       : TEXCOORD0;
};

static const float2 kPositions[3] = {
    float2( 0.0,  0.5),
    float2(-0.5, -0.5),
    float2( 0.5, -0.5),
};

static const float3 kColors[3] = {
    float3(1.0, 0.0, 0.0),
    float3(0.0, 1.0, 0.0),
    float3(0.0, 0.0, 1.0),
};

static const float2 kUVs[3] = {
    float2(0.5, 0.0),
    float2(0.0, 1.0),
    float2(1.0, 1.0),
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    float2 pos = kPositions[vertexID];
    output.position = mul(uniforms.mvp, float4(pos, 0.0, 1.0));
    output.color    = kColors[vertexID];
    output.uv       = kUVs[vertexID];
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    float3 color = input.color;
    color *= uniforms.tintColor.rgb;
    float alpha = uniforms.tintColor.a * (0.8 + 0.2 * sin(uniforms.time * 2.0));
    return float4(color, alpha);
}
)";
// NOLINTEND(*-avoid-c-arrays)

struct alignas(16) GpuUniforms
{
	float mvp[16];		// 64 bytes — identity matrix
	float tintColor[4]; // 16 bytes — RGBA tint
	float time;			// 4 bytes
	float pad[3];		// 12 bytes padding // NOLINT(*-avoid-c-arrays)
};

static_assert(sizeof(GpuUniforms) == 96, "GpuUniforms must be 96 bytes for GPU uniform alignment");

/// Helper: write an identity 4x4 matrix into a float[16] array (column-major).
static void MakeIdentityMatrix(float *out, size_t count)
{
	std::memset(out, 0, sizeof(float) * count);
	out[0] = 1.0f;
	out[5] = 1.0f;
	out[10] = 1.0f;
	out[15] = 1.0f;
}

class ExampleApp final : public Hush::IApplication
{
public:
	explicit ExampleApp(Hush::HushEngine *engine)
		: m_engine(engine),
		  m_scene(std::make_unique<Hush::Scene>(engine, engine->GetEngineThreadPool()))
	{
	}

	ExampleApp(const ExampleApp &) = delete;
	ExampleApp(ExampleApp &&) = delete;
	ExampleApp &operator=(const ExampleApp &) = delete;
	ExampleApp &operator=(ExampleApp &&) = delete;

	~ExampleApp() override = default;

	void Init() override
	{
		auto *renderGraphSystem = new Hush::Graphics::RenderGraphSystem( // NOLINT(*-owning-memory)
			*this->m_scene, &this->m_engine->GetWindowRenderer()->GetRenderDevice());
		this->m_scene->AddEngineSystem(renderGraphSystem);

		if (!InitShaderResources())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] ERROR: Failed to initialise shader resources!");
		}

		// ----------------------------------------------------------------
		// 3. Register a RenderGraph builder entity.
		// ----------------------------------------------------------------
		Hush::Entity renderGraphBuilderEntity = this->m_scene->CreateEntityWithName("RenderGraphBuilder");
		auto &builder = renderGraphBuilderEntity.AddComponent<Hush::RenderGraph::RenderGraphBuilderComponent>();

		builder.builderFunc = [this](Hush::RenderGraph::RenderGraph &graph) { this->SetupRenderGraph(graph); };

		builder.frameUpdateFunc = [this](Hush::RenderGraph::RenderGraph &graph) {
			this->UpdatePerFrameResources(graph);
		};

		this->m_scene->Init();
	}

	void Update(float delta) override
	{
		m_elapsedTime += delta;
		GetScene()->Update(delta);
	}

	void FixedUpdate(float delta) override
	{
		GetScene()->FixedUpdate(delta);
	}

	void OnPreRender() override
	{
		// Upload updated uniforms before the graph executes.
		UploadUniforms();
		GetScene()->PreRender();
	}

	void OnRender(float delta) override
	{
		GetScene()->Render();
	}

	void OnPostRender() override
	{
		GetScene()->PostRender();
	}

	void DisposeFrame() override
	{
	}

	[[nodiscard]]
	std::string_view GetAppName() const noexcept override
	{
		return "Hush Shader Triangle Demo";
	}

	Hush::Scene *GetScene() noexcept override
	{
		return this->m_scene.get();
	}

private:
	bool InitShaderResources()
	{
		using namespace Hush::Graphics;

		IGraphicsDevice *device = m_engine->GetWindowRenderer()->GetGraphicsDevice();
		if (device == nullptr)
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] No graphics device available.");
			return false;
		}

		// --- 1. Compile the shader with Slang --------------------------------

		ShaderCompilerOptions compilerOpts{};
		compilerOpts.target = ShaderCompiler::GetTargetForAPI(device->GetAPI());
		compilerOpts.optimizationLevel = 0; // No optimisation for easier debugging
		compilerOpts.generateDebugInfo = true;

		m_shaderCompiler = std::make_unique<ShaderCompiler>();
		if (!m_shaderCompiler->Initialize(compilerOpts))
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to initialise Slang shader compiler.");
			return false;
		}

		std::vector<ShaderEntryPointRequest> entryPoints = {
			{.stage = EShaderStage::Vertex, .entryPointName = "vertexMain"},
			{.stage = EShaderStage::Fragment, .entryPointName = "fragmentMain"},
		};

		ShaderCompilationResult compileResult =
			m_shaderCompiler->CompileFromSource(TRIANGLE_SHADER_SOURCE, "basic_triangle.slang", entryPoints);

		if (!compileResult.success)
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Shader compiled successfully.");
		if (!compileResult.diagnostics.empty())
		{
			Hush::LogFormat(Hush::ELogLevel::Info, "");
		}

		// --- 2. Create shader modules ----------------------------------------

		const auto *vsStage = compileResult.FindStage(EShaderStage::Vertex);
		const auto *fsStage = compileResult.FindStage(EShaderStage::Fragment);

		if (vsStage == nullptr || fsStage == nullptr)
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Missing vertex or fragment stage in compilation result.");
			return false;
		}

		m_vertexShader.CreateResource(vsStage->moduleDesc, device);
		m_fragmentShader.CreateResource(fsStage->moduleDesc, device);

		if (!m_vertexShader.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create vertex shader module.");
			return false;
		}
		if (!m_fragmentShader.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create fragment shader module.");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Shader modules created.");

		// --- 3. Create bind group layout from reflection ---------------------

		auto layoutDescs = compileResult.BuildBindGroupLayoutDescriptors();

		if (layoutDescs.empty())
		{
			// The shader has no bindings — create a minimal layout with the
			// uniform buffer entry manually (fallback).
			BindGroupLayoutDescriptor manualLayout{};
			manualLayout.debugName = "TriangleBindGroupLayout";
			BindGroupLayoutEntry uniformEntry{};
			uniformEntry.binding = 0;
			uniformEntry.type = EBindingType::UniformBuffer;
			uniformEntry.stageFlags = EShaderStageFlags::Vertex | EShaderStageFlags::Fragment;
			uniformEntry.minBufferBindingSize = sizeof(GpuUniforms);
			manualLayout.entries.push_back(uniformEntry);
			layoutDescs.push_back(std::move(manualLayout));
		}

		// Ensure minBufferBindingSize is set (reflection might report 0).
		for (auto &entry : layoutDescs[0].entries)
		{
			if (entry.type == EBindingType::UniformBuffer && entry.minBufferBindingSize == 0)
			{
				entry.minBufferBindingSize = sizeof(GpuUniforms);
			}
		}
		layoutDescs[0].debugName = "TriangleBindGroupLayout0";

		m_bindGroupLayout.CreateResource(layoutDescs[0], device);
		if (!m_bindGroupLayout.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create bind group layout.");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Bind group layout created.");

		BufferDescriptor bufferDesc{};
		bufferDesc.size = sizeof(GpuUniforms);
		bufferDesc.usage = EBufferUsage::Uniform;
		bufferDesc.memoryAccess = EMemoryAccess::CPUWrite;
		bufferDesc.debugName = "TriangleUniformBuffer";

		m_uniformBuffer.CreateResource(bufferDesc, device);
		if (!m_uniformBuffer.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create uniform buffer.");
			return false;
		}

		// Write initial uniform data.
		GpuUniforms initialUniforms{};
		MakeIdentityMatrix(static_cast<float *>(initialUniforms.mvp), 16);
		initialUniforms.tintColor[0] = 1.0f; // R
		initialUniforms.tintColor[1] = 1.0f; // G
		initialUniforms.tintColor[2] = 1.0f; // B
		initialUniforms.tintColor[3] = 1.0f; // A
		initialUniforms.time = 0.0f;

		device->WriteBuffer(m_uniformBuffer.Get(), 0, &initialUniforms, sizeof(initialUniforms));

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Uniform buffer created and initialised.");

		BindGroupDescriptor bgDesc{};
		bgDesc.layout = m_bindGroupLayout.Get();
		bgDesc.debugName = "TriangleBindGroup";

		BindGroupEntry bufEntry{};
		bufEntry.binding = 0;
		bufEntry.buffer = m_uniformBuffer.Get();
		bufEntry.offset = 0;
		bufEntry.size = sizeof(GpuUniforms);
		bgDesc.entries.push_back(bufEntry);

		m_bindGroup.CreateResource(bgDesc, device);
		if (!m_bindGroup.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create bind group.");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Bind group created.");

		// --- 6. Create the graphics pipeline ---------------------------------

		GraphicsPipelineDescriptor pipelineDesc{};
		pipelineDesc.debugName = "TrianglePipeline";

		// Vertex stage
		pipelineDesc.vertexStage.module = m_vertexShader.Get();
		pipelineDesc.vertexStage.entryPoint = "vertexMain";

		// Fragment stage
		pipelineDesc.fragmentStage.module = m_fragmentShader.Get();
		pipelineDesc.fragmentStage.entryPoint = "fragmentMain";

		pipelineDesc.primitive.topology = EPrimitiveTopology::TriangleList;
		pipelineDesc.primitive.cullMode = ECullModeFlags::None;

		ColorTargetState colorTarget{};
		colorTarget.format = ETextureFormat::BGRA8_UNORM;

		colorTarget.blendEnabled = true;
		colorTarget.colorBlend.srcFactor = EBlendFactor::SrcAlpha;
		colorTarget.colorBlend.dstFactor = EBlendFactor::OneMinusSrcAlpha;
		colorTarget.colorBlend.operation = EBlendOperation::Add;
		colorTarget.alphaBlend.srcFactor = EBlendFactor::One;
		colorTarget.alphaBlend.dstFactor = EBlendFactor::OneMinusSrcAlpha;
		colorTarget.alphaBlend.operation = EBlendOperation::Add;
		colorTarget.writeMask = EColorWriteMask::All;

		pipelineDesc.colorTargets.push_back(colorTarget);

		// No depth/stencil for this simple demo.
		pipelineDesc.depthStencil.enabled = false;

		// Bind group layouts — one group at set 0.
		pipelineDesc.bindGroupLayouts[0] = m_bindGroupLayout.Get();
		pipelineDesc.bindGroupLayoutCount = 1;

		m_pipeline.CreateResource(pipelineDesc, device);
		if (!m_pipeline.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create graphics pipeline.");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Graphics pipeline created successfully.");
		return true;
	}

	void UploadUniforms()
	{
		if (!m_uniformBuffer.IsValid())
		{
			return;
		}

		IGraphicsDevice *device = m_engine->GetWindowRenderer()->GetGraphicsDevice();
		if (device == nullptr)
		{
			return;
		}

		GpuUniforms uniforms{};
		MakeIdentityMatrix(static_cast<float *>(uniforms.mvp), 16);
		uniforms.tintColor[0] = 1.0f;
		uniforms.tintColor[1] = 1.0f;
		uniforms.tintColor[2] = 1.0f;
		uniforms.tintColor[3] = 1.0f;
		uniforms.time = m_elapsedTime;

		device->WriteBuffer(m_uniformBuffer.Get(), 0, &uniforms, sizeof(uniforms));
	}

	void SetupRenderGraph(Hush::RenderGraph::RenderGraph &graph)
	{
		using namespace Hush::RenderGraph;
		using namespace Hush::Graphics;

		IGraphicsDevice *device = m_engine->GetWindowRenderer()->GetGraphicsDevice();

		// ----------------------------------------------------------------
		// Pass 1: TrianglePass — clear the render texture and draw a triangle.
		// ----------------------------------------------------------------
		struct TrianglePassData
		{
			ResourceId renderTexture;
		};

		const auto &trianglePassData = graph.AddPass<TrianglePassData>(
			EPassType::Graphics, "TrianglePass",

			// BUILD PHASE -------------------------------------------------
			[this](RenderGraph::BuildContext &ctx, TrianglePassData &data) {
				const auto windowSize = this->m_engine->GetWindowRenderer()->GetWindowSize();

				data.renderTexture = ctx.Create<TextureResource>(
					"TrianglePass_RenderTexture", TextureDescriptor{
													  .width = static_cast<uint32_t>(windowSize.x),
													  .height = static_cast<uint32_t>(windowSize.y),
													  .format = ETextureFormat::BGRA8_UNORM,
													  .usage = ETextureUsage::RenderTarget | ETextureUsage::CopySource,
												  });
			},

			// EXECUTE PHASE -----------------------------------------------
			[this](TrianglePassData &data, Hush::Graphics::ICommandList *cmdList,
				   const Hush::RenderGraph::ResourceManager &resourceManager) {
				auto *cmd = dynamic_cast<Hush::Graphics::IGraphicsCommandList *>(cmdList);
				if (cmd == nullptr)
				{
					Hush::LogFormat(Hush::ELogLevel::Error, "[TrianglePass] Failed to get graphics command list.");
					return;
				}

				// Build render pass descriptor --------------------------------
				RenderPassDescriptor renderPass{};
				renderPass.debugLabel = "TrianglePass";

				RenderPassColorAttachment colorAttachment{};
				colorAttachment.texture =
					resourceManager.GetResource<TextureResource>(data.renderTexture)->texture.get();
				colorAttachment.loadOp = ELoadOp::Clear;
				colorAttachment.storeOp = EStoreOp::Store;
				colorAttachment.clearValue = ClearColorValue{0.05f, 0.05f, 0.08f, 1.0f};
				renderPass.AddColorAttachment(colorAttachment);

				// Begin render pass -------------------------------------------
				cmd->BeginRenderPass(renderPass);

				// Bind pipeline & resources -----------------------------------
				if (m_pipeline.IsValid() && m_bindGroup.IsValid())
				{
					cmd->BindPipeline(m_pipeline.Get());
					cmd->SetBindGroup(0, m_bindGroup.Get());

					// Set viewport & scissor to match the render target.
					auto *tex = resourceManager.GetResource<TextureResource>(data.renderTexture)->texture.get();
					auto w = static_cast<float>(tex->GetWidth());
					auto h = static_cast<float>(tex->GetHeight());
					cmd->SetViewport(0.0f, 0.0f, w, h, 0.0f, 1.0f);
					cmd->SetScissor(0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h));

					// Draw the triangle — 3 vertices, 1 instance, no vertex buffer.
					cmd->Draw(3, 1, 0, 0);
				}

				cmd->EndRenderPass();
			});

		// ----------------------------------------------------------------
		// Pass 2: CopyToBackbuffer — copy the render texture to the swapchain.
		// ----------------------------------------------------------------
		struct CopyToBackbufferPassData
		{
			ResourceId renderTexture;
			ResourceId backbuffer;
		};

		graph.AddPass<CopyToBackbufferPassData>(
			EPassType::Transfer, "CopyToBackbuffer",

			// BUILD PHASE -------------------------------------------------
			[&trianglePassData, device, this](RenderGraph::BuildContext &ctx, CopyToBackbufferPassData &data) {
				data.renderTexture = ctx.Read(trianglePassData.renderTexture);
				data.backbuffer =
					ctx.Import<ImportedTextureResource>("Backbuffer", ImportedTextureResource{
																		  .texture = device->GetCurrentFrameTexture(),
																	  });
				m_backbufferResourceId = data.backbuffer;
			},

			// EXECUTE PHASE -----------------------------------------------
			[](CopyToBackbufferPassData &data, Hush::Graphics::ICommandList *cmdList,
			   const Hush::RenderGraph::ResourceManager &resourceManager) {
				auto *cmd =
					static_cast<Hush::Graphics::ICopyCommandList *>(cmdList); // NOLINT(*-pro-type-static-cast-downcast)

				auto *sourceTexture = resourceManager.GetResource<TextureResource>(data.renderTexture)->texture.get();
				auto *destinationTexture =
					resourceManager.GetResource<ImportedTextureResource>(data.backbuffer)->texture;

				cmd->CopyTexture(sourceTexture, 0, 0, 0, destinationTexture, 0, 0, 0,

								 sourceTexture->GetWidth(), sourceTexture->GetHeight(), 1);
			});
	}

	void UpdatePerFrameResources(Hush::RenderGraph::RenderGraph &graph)
	{
		using namespace Hush::Graphics;

		IGraphicsDevice *device = m_engine->GetWindowRenderer()->GetGraphicsDevice();

		graph.UpdateImport<ImportedTextureResource>(m_backbufferResourceId,
													ImportedTextureResource{
														.texture = device->GetCurrentFrameTexture(),
													});
	}

	Hush::HushEngine *m_engine;
	std::unique_ptr<Hush::Scene> m_scene;

	// Shader compilation
	std::unique_ptr<Hush::Graphics::ShaderCompiler> m_shaderCompiler;

	// GPU objects — owned by GraphicsResource wrappers for automatic lifetime management.
	Hush::Graphics::ShaderResource m_vertexShader;
	Hush::Graphics::ShaderResource m_fragmentShader;
	Hush::Graphics::BindGroupLayoutResource m_bindGroupLayout;
	Hush::Graphics::BindGroupResource m_bindGroup;
	Hush::Graphics::GraphicsPipelineResource m_pipeline;
	Hush::Graphics::BufferResource m_uniformBuffer;

	// Render graph bookkeeping
	Hush::RenderGraph::ResourceId m_backbufferResourceId{0};

	// Animation
	float m_elapsedTime = 0.0f;

	// IGraphicsDevice shortcut (avoids repeated lookups).
	using IGraphicsDevice = Hush::Graphics::IGraphicsDevice;
};

extern "C" bool BundledAppExists_Internal_() // NOLINT(*-identifier-naming)
{
	return true;
}

extern "C" Hush::IApplication *BundledApp_Internal_(Hush::HushEngine *engine) // NOLINT(*-identifier-naming)
{
	return new ExampleApp(engine); // NOLINT(*-owning-memory)
}
