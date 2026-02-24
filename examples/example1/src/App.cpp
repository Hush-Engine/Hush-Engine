#include "Components/TextureComponent.hpp"
#include "Components/RenderGraphBuilderComponent.hpp"
#include "HushEngine.hpp"
#include "WindowRenderer.hpp"
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
#include "ResourceManager.hpp"
#include "Scene.hpp"
#include "WindowRenderer.hpp"
#include <cstring>
#include <iostream>
#include <memory>

// NOLINTBEGIN(*-avoid-c-arrays)
static constexpr const char *FULLSCREEN_SHADER_SOURCE = R"(
struct Uniforms
{
    float4x4 mvp;
    float4   tintColor;
    float    time;
};

[[vk::binding(0, 0)]]
ConstantBuffer<Uniforms> uniforms;

[[vk::binding(1, 0)]]
Texture2D<float4> tex;

[[vk::binding(2, 0)]]
SamplerState texSampler;

struct VertexOutput
{
    float4 position : SV_Position;
    float2 uv       : TEXCOORD0;
};

// Centered half-size quad as two triangles (6 vertices, CCW winding)
static const float2 kPositions[6] = {
    float2(-0.5,  0.5),   // top-left
    float2(-0.5, -0.5),   // bottom-left
    float2( 0.5, -0.5),   // bottom-right

    float2(-0.5,  0.5),   // top-left
    float2( 0.5, -0.5),   // bottom-right
    float2( 0.5,  0.5),   // top-right
};

static const float2 kUVs[6] = {
    float2(0.0, 0.0),     // top-left
    float2(0.0, 1.0),     // bottom-left
    float2(1.0, 1.0),     // bottom-right

    float2(0.0, 0.0),     // top-left
    float2(1.0, 1.0),     // bottom-right
    float2(1.0, 0.0),     // top-right
};

[shader("vertex")]
VertexOutput vertexMain(uint vertexID : SV_VertexID)
{
    VertexOutput output;
    output.position = float4(kPositions[vertexID], 0.0, 1.0);
    output.uv       = kUVs[vertexID];
    return output;
}

[shader("fragment")]
float4 fragmentMain(VertexOutput input) : SV_Target
{
    return tex.Sample(texSampler, input.uv);
}
)";
// NOLINTEND(*-avoid-c-arrays)

struct alignas(16) GpuUniforms
{

	glm::mat4 mvp;		 // 64 bytes — identity matrix
	glm::vec4 tintColor; // 16 bytes — RGBA tint (alternative using glm::vec4 for easier manipulation in C++)
	float time;			 // 4 bytes
	float pad[3];		 // 12 bytes padding // NOLINT(*-avoid-c-arrays)
};

static_assert(sizeof(GpuUniforms) == 96, "GpuUniforms must be 96 bytes for GPU uniform alignment");

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
		if (!InitShaderResources())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] ERROR: Failed to initialise shader resources!");
		}

		this->m_scene->Init();

		Hush::Entity renderGraphBuilderEntity = this->m_scene->CreateEntityWithName("RenderGraphBuilder");
		auto &builder = renderGraphBuilderEntity.AddComponent<Hush::RenderGraph::RenderGraphBuilderComponent>();

		builder.builderFunc = [this](Hush::RenderGraph::RenderGraph &graph) { this->SetupRenderGraph(graph); };

		builder.frameUpdateFunc = [this](Hush::RenderGraph::RenderGraph &graph) {
			this->UpdatePerFrameResources(graph);
		};

		// Load the cat texture and keep a reference for later binding
		auto textureEntity = this->m_scene->CreateEntity();
		auto textureResult = m_engine->GetResourceManager()->LoadTexture("engine_res://resources/cat.jpg");
		HUSH_ASSERT(textureResult.has_value(), "Failed to load texture resource!");
		m_textureRef = textureResult.value();
		textureEntity.EmplaceComponent<Hush::Ref<Hush::TextureComponent>>(m_textureRef);
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
		// Try to create the bind group if the texture has been uploaded to the GPU
		EnsureBindGroup();
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
		return "Hush Textured Triangle Demo";
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
			m_shaderCompiler->CompileFromSource(FULLSCREEN_SHADER_SOURCE, "fullscreen_texture.slang", entryPoints);

		if (!compileResult.success)
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Shader compilation failed.");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Shader compiled successfully.");
		if (!compileResult.diagnostics.empty())
		{
			Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Shader diagnostics: {}", compileResult.diagnostics);
		}

		const auto *vsStage = compileResult.FindStage(EShaderStage::Vertex);
		const auto *fsStage = compileResult.FindStage(EShaderStage::Fragment);

		if (vsStage == nullptr || fsStage == nullptr)
		{
			Hush::LogFormat(Hush::ELogLevel::Error,
							"[ExampleApp] Missing vertex or fragment stage in compilation result.");
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

		m_uniformBuffer.CreateResource(
			BufferDescriptor{
				.size = sizeof(GpuUniforms),
				.usage = EBufferUsage::Uniform | EBufferUsage::CopyDestination,
				.memoryAccess = EMemoryAccess::CPUNone,
				.debugName = "ExampleApp_Uniforms",
			},
			device);

		if (!m_uniformBuffer.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create uniform buffer.");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Uniform buffer created.");

		if (!CreateNativeSampler(device))
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create sampler.");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Sampler created.");

		BindGroupLayoutDescriptor layoutDesc{};
		layoutDesc.debugName = "ExampleApp_BindGroupLayout";
		layoutDesc.entries = {
			// binding 0: Uniform buffer
			BindGroupLayoutEntry{
				.binding = 0,
				.type = EBindingType::UniformBuffer,
				.stageFlags = EShaderStageFlags::Vertex | EShaderStageFlags::Fragment,
				.minBufferBindingSize = sizeof(GpuUniforms),
			},
			// binding 1: Sampled texture
			BindGroupLayoutEntry{
				.binding = 1,
				.type = EBindingType::SampledTexture,
				.stageFlags = EShaderStageFlags::Fragment,
				.textureSampleType = ETextureSampleType::Float,
				.textureViewDimension = 2, // 2D
			},
			// binding 2: Sampler
			BindGroupLayoutEntry{
				.binding = 2,
				.type = EBindingType::Sampler,
				.stageFlags = EShaderStageFlags::Fragment,
				.samplerType = ESamplerBindingType::Filtering,
			},
		};

		m_bindGroupLayout.CreateResource(layoutDesc, device);

		if (!m_bindGroupLayout.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create bind group layout.");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Bind group layout created.");

		GraphicsPipelineDescriptor pipelineDesc{};
		pipelineDesc.debugName = "ExampleApp_TexturedTrianglePipeline";

		pipelineDesc.vertexStage = PipelineShaderStage{
			.module = m_vertexShader.Get(),
			.entryPoint = "vertexMain",
		};
		pipelineDesc.fragmentStage = PipelineShaderStage{
			.module = m_fragmentShader.Get(),
			.entryPoint = "fragmentMain",
		};

		// No vertex buffer layouts — we generate vertices in the shader via SV_VertexID
		pipelineDesc.primitive = PrimitiveState{
			.topology = EPrimitiveTopology::TriangleList,
			.cullMode = ECullModeFlags::None,
		};

		// Single color target matching the render texture format
		ColorTargetState colorTarget{};
		colorTarget.format = ETextureFormat::BGRA8_UNORM;
		colorTarget.blendEnabled = true;
		colorTarget.colorBlend = BlendComponent{
			.operation = EBlendOperation::Add,
			.srcFactor = EBlendFactor::SrcAlpha,
			.dstFactor = EBlendFactor::OneMinusSrcAlpha,
		};
		colorTarget.alphaBlend = BlendComponent{
			.operation = EBlendOperation::Add,
			.srcFactor = EBlendFactor::One,
			.dstFactor = EBlendFactor::OneMinusSrcAlpha,
		};
		colorTarget.writeMask = EColorWriteMask::All;
		pipelineDesc.colorTargets.push_back(colorTarget);

		// Bind group layout
		pipelineDesc.bindGroupLayouts[0] = m_bindGroupLayout.Get();
		pipelineDesc.bindGroupLayoutCount = 1;

		m_pipeline.CreateResource(pipelineDesc, device);

		if (!m_pipeline.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create graphics pipeline.");
			return false;
		}

		Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Graphics pipeline created.");

		return true;
	}

	bool CreateNativeSampler(Hush::Graphics::IGraphicsDevice *device)
	{
		using namespace Hush::Graphics;

		SamplerDescriptor samplerDesc{};
		samplerDesc.debugName = "ExampleApp_Sampler";
		samplerDesc.addressModeU = EAddressMode::ClampToEdge;
		samplerDesc.addressModeV = EAddressMode::ClampToEdge;
		samplerDesc.addressModeW = EAddressMode::ClampToEdge;
		samplerDesc.magFilter = EFilterMode::Linear;
		samplerDesc.minFilter = EFilterMode::Linear;
		samplerDesc.mipmapFilter = EFilterMode::Linear;
		samplerDesc.lodMinClamp = 0.0f;
		samplerDesc.lodMaxClamp = 32.0f;
		samplerDesc.compare = ECompareFunction::Undefined;
		samplerDesc.maxAnisotropy = 1;

		m_sampler.CreateResource(samplerDesc, device);
		return m_sampler.IsValid();
	}

	/// @brief Lazily create the bind group once the texture has been uploaded to the GPU.
	void EnsureBindGroup()
	{
		// Already created
		if (m_bindGroup.IsValid())
		{
			return;
		}

		// Check prerequisites
		if (!m_uniformBuffer.IsValid() || !m_bindGroupLayout.IsValid())
		{
			return;
		}

		// Check that the texture has been uploaded
		if (m_textureRef.IsNull() || !m_textureRef->IsValid() || m_textureRef->GetGpuTexture() == nullptr)
		{
			return;
		}

		if (!m_sampler.IsValid())
		{
			return;
		}

		using namespace Hush::Graphics;

		IGraphicsDevice *device = m_engine->GetWindowRenderer()->GetGraphicsDevice();
		if (device == nullptr)
		{
			return;
		}

		BindGroupDescriptor bgDesc{};
		bgDesc.layout = m_bindGroupLayout.Get();
		bgDesc.debugName = "ExampleApp_BindGroup";
		bgDesc.entries = {
			// binding 0: uniform buffer
			BindGroupEntry{
				.binding = 0,
				.buffer = m_uniformBuffer.Get(),
				.offset = 0,
				.size = sizeof(GpuUniforms),
			},
			// binding 1: sampled texture
			BindGroupEntry{
				.binding = 1,
				.texture = m_textureRef->GetGpuTexture(),
			},
			// binding 2: sampler
			BindGroupEntry{
				.binding = 2,
				.sampler = m_sampler.Get(),
			},
		};

		m_bindGroup.CreateResource(bgDesc, device);

		if (m_bindGroup.IsValid())
		{
			Hush::LogFormat(Hush::ELogLevel::Info, "[ExampleApp] Bind group created — texture is ready for rendering.");
		}
		else
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "[ExampleApp] Failed to create bind group.");
		}
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
		uniforms.mvp = glm::mat4(1.0f); // Identity matrix for MVP
		uniforms.tintColor = glm::vec4(1.0f);
		uniforms.time = m_elapsedTime;

		device->WriteBuffer(m_uniformBuffer.Get(), 0, &uniforms, sizeof(uniforms));
	}

	void SetupRenderGraph(Hush::RenderGraph::RenderGraph &graph)
	{
		using namespace Hush::RenderGraph;
		using namespace Hush::Graphics;

		IGraphicsDevice *device = m_engine->GetWindowRenderer()->GetGraphicsDevice();

		struct TrianglePassData
		{
			ResourceId renderTexture;
		};

		const auto &trianglePassData = graph.AddPass<TrianglePassData>(
			EPassType::Graphics, "TrianglePass",

			[this](RenderGraph::BuildContext &ctx, TrianglePassData &data) {
				const auto windowSize = this->m_engine->GetWindowRenderer()->GetWindowSize();

				ctx.Read(ctx.GetResourceIdByName(RenderGraph::RenderGraph::RESOURCE_UPLOAD_SYNC_TOKEN_NAME));

				data.renderTexture = ctx.Create<TextureResource>(
					"TrianglePass_RenderTexture", TextureDescriptor{
													  .width = static_cast<uint32_t>(windowSize.x),
													  .height = static_cast<uint32_t>(windowSize.y),
													  .format = ETextureFormat::BGRA8_UNORM,
													  .usage = ETextureUsage::RenderTarget | ETextureUsage::CopySource,
												  });
			},

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

				// Draw the textured triangle if resources are ready ------------
				if (m_pipeline.IsValid() && m_bindGroup.IsValid())
				{
					cmd->BindPipeline(m_pipeline.Get());
					cmd->SetBindGroup(0, m_bindGroup.Get());
					cmd->Draw(6, 1, 0, 0); // 6 vertices (fullscreen quad), 1 instance
				}

				cmd->EndRenderPass();
			});

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

				cmd->CopyTexture(sourceTexture, 0, 0, 0, destinationTexture, 0, 0, 0, sourceTexture->GetWidth(),
								 sourceTexture->GetHeight(), 1);
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

	// Texture reference — kept alive so we can access the GPU texture for binding
	Hush::Ref<Hush::TextureComponent> m_textureRef;

	// Sampler — owned by SamplerResource wrapper for automatic lifetime management.
	Hush::Graphics::SamplerResource m_sampler;

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
