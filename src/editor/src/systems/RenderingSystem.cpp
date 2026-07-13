#include "RenderingSystem.hpp"
#include "Assertions.hpp"
#include "Components/GlobalKeys.hpp"
#include "Components/MeshReference.hpp"
#include "Components/RenderGraphBuilderComponent.hpp"
#include "Components/WorldTransform.hpp"
#include "HushEngine.hpp"
#include "Logger.hpp"
#include "Profiling.hpp"
#include "Query.hpp"
#include "RHI/GraphicsResources.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "RHI/ICommandList.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include "RHI/IShaderModule.hpp"
#include "RHI/PipelineDescriptor.hpp"
#include "RHI/ShaderCompiler.hpp"
#include "RHI/ISampler.hpp"
#include "RHI/ICommandQueue.hpp"
#include "../components/EditorPanelComponents.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Scene.hpp"
#include "Shared/EditorCamera.hpp"
#include "Shared/PBRMaterial.hpp"
#include "VirtualFilesystem.hpp"
#include "WindowManager.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/matrix.hpp>
#include <cstddef>
#include <cstring>
#include <string_view>

using namespace Hush::Graphics;

void Hush::RenderingSystem::BuildScenePassFunction(Hush::RenderGraph::RenderGraph &graph, Hush::RenderingSystem *self)
{
	using namespace Hush::Graphics;
	using namespace Hush::RenderGraph;

	using RenderGraphBuildContext_t = Hush::RenderGraph::RenderGraph::BuildContext;

	struct ScenePassData
	{
		ResourceId renderTexture;
		ResourceId depthTexture;
	};

	graph.AddPass<ScenePassData>(
		EPassType::Graphics, "EditorScenePass",
		[self](RenderGraphBuildContext_t &ctx, ScenePassData &data) {
			const glm::u32vec2 bufferSize = self->m_cachedViewportSize;

			ctx.Read(ctx.GetResourceIdByName(Hush::RenderGraph::RenderGraph::RESOURCE_UPLOAD_SYNC_TOKEN_NAME));

			data.renderTexture = ctx.Create<TextureResource>(
				"EditorScenePass_RenderTexture", TextureDescriptor{
													 .width = bufferSize.x,
													 .height = bufferSize.y,
													 .format = ETextureFormat::BGRA8_UNORM,
													 .usage = ETextureUsage::RenderTarget | ETextureUsage::Sampled,
												 });

			data.depthTexture =
				ctx.Create<TextureResource>("EditorScenePass_DepthTexture", TextureDescriptor{
																				.width = bufferSize.x,
																				.height = bufferSize.y,
																				.format = ETextureFormat::D32_FLOAT,
																				.usage = ETextureUsage::DepthStencil,
																			});

			ctx.SetCullingMode(RenderPassNode::EPassCullingMode::NeverCull);
		},
		[self](ScenePassData &data, ICommandList *cmdList, const Hush::RenderGraph::ResourceManager &resourceManager) {
			auto *cmd = dynamic_cast<IGraphicsCommandList *>(cmdList);
			if (cmd == nullptr)
			{
				Hush::LogFormat(Hush::ELogLevel::Error, "[EditorScenePass] Failed to get graphics command list.");
				return;
			}

			RenderPassDescriptor renderPass{};
			renderPass.debugLabel = "EditorScenePass";

			RenderPassColorAttachment colorAttachment{};
			colorAttachment.texture = resourceManager.GetResource<TextureResource>(data.renderTexture)->texture.get();
			colorAttachment.loadOp = ELoadOp::Clear;
			colorAttachment.storeOp = EStoreOp::Store;
			colorAttachment.clearValue = ClearColorValue{0.0f, 0.0f, 0.0f, 0.0f};
			renderPass.AddColorAttachment(colorAttachment);

			RenderPassDepthStencilAttachment depthAttachment{};
			depthAttachment.texture = resourceManager.GetResource<TextureResource>(data.depthTexture)->texture.get();
			depthAttachment.depthLoadOp = ELoadOp::Clear;
			depthAttachment.depthStoreOp = EStoreOp::Store;
			depthAttachment.depthClearValue = 1.0f;
			renderPass.SetDepthStencilAttachment(depthAttachment);

			auto *graphicsDevice = WindowManager::GetMainWindow()->GetGraphicsDevice();

			graphicsDevice->WriteBuffer(self->m_sceneDataBuffer.get(), 0, &self->m_cachedSceneData, sizeof(SceneData));

			cmd->BeginRenderPass(renderPass);
			cmd->SetViewport(0, 0, static_cast<float>(self->m_cachedViewportSize.x),
							 static_cast<float>(self->m_cachedViewportSize.y), 0, 1);

			graphicsDevice->WriteBuffer(self->m_gridUniformBuffer.get(), 0, &self->m_cachedViewUniforms,
										sizeof(GridViewUniforms));
			cmd->BindPipeline(self->m_gridPipeline.get());
			cmd->SetBindGroup(0, self->m_gridBindGroup.get());
			cmd->Draw(6, 1, 0, 0);

			cmd->BindPipeline(self->m_meshPipeline.get());

			for (const auto &draw : self->m_meshDrawList)
			{
				cmd->SetVertexBuffer(0, draw.vertexBuffer);
				cmd->SetIndexBuffer(draw.indexBuffer);
				cmd->SetBindGroup(0, self->m_meshSceneBindGroup.get(),
								  std::span<const uint32_t>{&draw.dynamicOffset, 1});
				if (draw.materialBindGroup != nullptr)
				{
					cmd->SetBindGroup(1, draw.materialBindGroup);
				}
				cmd->DrawIndexed(draw.indexCount, 1, draw.firstIndex, 0, 0);
			}

			cmd->EndRenderPass();
		});
}

void Hush::RenderingSystem::Init()
{
	// Weird, but this is how flecs creates systems, they are associated with an entity and we can query for them
	Entity selfEntity = this->GetScene().CreateEntityWithKey("RenderingSystem");
	auto &renderingSystemRef = selfEntity.AddComponent<RenderingSystem *>();
	renderingSystemRef = this;

	// TODO: Check why we can't do Cache::All
	this->m_renderableTargetsQuery =
		this->GetScene().CreateQuery<const MeshReference, const WorldTransform>(RawQuery::ECacheMode::Auto);

	this->GetScene().AddComponentObserver<ScenePanelSizeComp>(
		EComponentObserverType::Set, [this](Entity::EntityId entity, ScenePanelSizeComp *panelSize) {
			(void)entity;
			this->m_cachedViewportSize = panelSize->size;
		});

	this->m_editorCameraQuery = this->GetScene().CreateQuery<EditorCamera>();

	// Make sure we have the data so that initialization order does not matter
	auto scenePanelQuery = this->GetScene().CreateQuery<const ScenePanelSizeComp>(RawQuery::ECacheMode::None);
	scenePanelQuery.Each([this](const ScenePanelSizeComp &sizeComp) { this->m_cachedViewportSize = sizeComp.size; });

	// We need to load the shaders here
	// Access the filesystem
	Entity engineManager = this->GetScene().CreateEntityWithKey(ENGINE_MANAGER);

	VirtualFilesystem *vfs = this->GetScene().GetEngine()->GetVirtualFilesystem();
	HUSH_ASSERT(vfs != nullptr, "File system on engine manager can't be null, check initialization order!");

	auto *shaderCompiler = engineManager.GetComponent<Hush::Graphics::ShaderCompiler>();
	HUSH_ASSERT(shaderCompiler != nullptr,
				"Shader compiler on engine manager can't be null, check initialization order!");

	shaderCompiler->Initialize({.matrixLayout = 1});

	Graphics::IGraphicsDevice *device = WindowManager::GetMainWindow()->GetGraphicsDevice();

	SetupGridPipeline(device, vfs, shaderCompiler);
	this->m_pbrCompilationData = SetupMeshPipeline(device, vfs, shaderCompiler);

	Entity sceneBuilderEnt = this->GetScene().CreateEntityWithKey("SceneRenderGraph");
	auto &scenePassBuilder = sceneBuilderEnt.AddComponent<RenderGraph::RenderGraphBuilderComponent>();
	scenePassBuilder.builderFunc = [this](RenderGraph::RenderGraph &graph) { BuildScenePassFunction(graph, this); };
	scenePassBuilder.frameUpdateFunc = [](Hush::RenderGraph::RenderGraph &) {};

	this->m_pbrMaterialDescriptor = {
		.vertexShader = this->m_meshVertModule.get(),
		.fragmentShader = this->m_meshFragModule.get(),
		.vertexEntry = "vertMain",
		.fragmentEntry = "fragMain",
		.compilationResult = &this->m_pbrCompilationData,
		.colorTargetFormat = ETextureFormat::BGRA8_UNORM,
	};
}

void Hush::RenderingSystem::OnShutdown()
{
	m_materialBindGroupCache.clear();
}

void Hush::RenderingSystem::OnUpdate([[maybe_unused]] float delta)
{
}

void Hush::RenderingSystem::OnFixedUpdate([[maybe_unused]] float delta)
{
}

void Hush::RenderingSystem::OnRender()
{
}

void Hush::RenderingSystem::OnPreRender()
{
	ZoneScoped;

	this->m_editorCameraQuery.Each([this](Entity::EntityId ent, EditorCamera &editorCam) {
		(void)ent;
		glm::mat4 view = editorCam.GetViewMatrix();
		glm::mat4 proj = editorCam.GetProjectionMatrix();
		glm::mat4 viewProj = proj * view;

		// Grid uniforms
		this->m_cachedViewUniforms.resolution = {this->m_cachedViewportSize.x, this->m_cachedViewportSize.y};
		this->m_cachedViewUniforms.invViewProj = glm::inverse(viewProj);
		this->m_cachedViewUniforms.farPlane = editorCam.GetFarPlane();

		glm::vec3 pos = editorCam.GetPosition();
		this->m_cachedViewUniforms.pos = glm::vec4(pos.x, pos.y, pos.z, 1.0);

		// Scene data for mesh rendering
		this->m_cachedSceneData.view = view;
		this->m_cachedSceneData.proj = proj;
		this->m_cachedSceneData.viewproj = viewProj;
		this->m_cachedSceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.15f, 1.0f);
		this->m_cachedSceneData.sunlightDirection = glm::vec4(0.5f, 1.0f, 0.3f, 1.0f);
		this->m_cachedSceneData.sunlightColor = glm::vec4(1.0f, 0.98f, 0.9f, 1.0f);
	});

	m_meshDrawList.clear();

	HUSH_ASSERT(this->m_meshPipeline != nullptr, "Mesh pipeline should not ever be null!");

	Graphics::IGraphicsDevice *device = WindowManager::GetMainWindow()->GetGraphicsDevice();
	uint32_t slotIndex = 0;

	size_t meshCount = this->m_renderableTargetsQuery.begin().Size();

	// Resize our mesh buffer if we get to the max amount of meshes
	uint64_t currBufferSize = this->m_meshModelBuffer->GetSize();
	constexpr uint64_t meshBufferGrowthFactor = 2;
	if (meshCount > (currBufferSize / this->m_meshModelSlotSize)) {
		device->ResizeBuffer(this->m_meshModelBuffer.get(), currBufferSize * meshBufferGrowthFactor);
	}

	m_renderableTargetsQuery.Each([this, device, &slotIndex](const MeshReference &meshRef,
															 const WorldTransform &xform) {
		const auto *mesh = meshRef.GetMesh().Get();
		if (mesh == nullptr)
		{
			return;
		}

		auto *vb = meshRef.GetGpuVertexBuffer();
		auto *ib = meshRef.GetGpuIndexBuffer();
		if (vb == nullptr || ib == nullptr)
		{
			return;
		}

		glm::mat4 modelMatrix = xform.GetTransformationMatrix();

		uint32_t slotOffset = slotIndex * m_meshModelSlotSize;
		device->WriteBuffer(m_meshModelBuffer.get(), slotOffset, &modelMatrix, sizeof(glm::mat4));

		for (const auto &surface : mesh->GetSurfaces())
		{
			Graphics::IBindGroup *matBindGroup = nullptr;
			auto *mat = surface.material;
			if (mat != nullptr)
			{
				mat->FlushProperties(device);
				auto it = m_materialBindGroupCache.find(mat);
				if (it == m_materialBindGroupCache.end())
				{
					BindGroupDescriptor bgDesc{};
					bgDesc.layout = m_meshMaterialBindGroupLayout.get();
					bgDesc.debugName = mat->GetName() + "_BindGroup";
					uint64_t bufSize = mat->GetUniformBufferSize();
					bgDesc.entries = {{.binding = 0, .buffer = mat->GetUniformBuffer(), .offset = 0, .size = bufSize},
									  {.binding = 1, .texture = m_defaultColorTex.get()},
									  {.binding = 2, .texture = m_defaultMetalRoughTex.get()},
									  {.binding = 3, .texture = m_defaultNormalTex.get()},
									  {.binding = 4, .texture = m_defaultEmissiveTex.get()},
									  {.binding = 5, .sampler = m_defaultSampler.get()}};
					auto [newIt, _] = m_materialBindGroupCache.emplace(mat, device->CreateBindGroup(bgDesc));
					it = newIt;
				}
				matBindGroup = it->second.get();
			}
			else
			{
				matBindGroup = m_meshMaterialBindGroup.get();
			}

			m_meshDrawList.push_back(MeshDraw{
				.modelMatrix = modelMatrix,
				.vertexBuffer = vb,
				.indexBuffer = ib,
				.indexCount = surface.count,
				.firstIndex = surface.startIndex,
				.dynamicOffset = slotOffset,
				.materialBindGroup = matBindGroup,
			});
		}

		slotIndex++;
	});
}

void Hush::RenderingSystem::OnPostRender()
{
}

std::string_view Hush::RenderingSystem::GetName() const
{
	return "RenderingSystem";
}

Hush::Graphics::Material3DDescriptor &Hush::RenderingSystem::GetPBRDescriptor()
{
	return this->m_pbrMaterialDescriptor;
}

void Hush::RenderingSystem::SetupGridPipeline(Graphics::IGraphicsDevice *device, VirtualFilesystem *vfs,
											  Graphics::ShaderCompiler *shaderCompiler)
{
	// With the filesystem
	constexpr std::string_view virtualPath = "engine_res://res/shaders/grid.slang";
	auto res = vfs->ResolveVirtualPath(virtualPath);
	HUSH_RESULT_ASSERT(res, "Could not load grid shader! Make sure it's present on {}", virtualPath);

	std::string_view actualPath = res.value();

	Graphics::ShaderCompilationResult compilationResult = shaderCompiler->CompileFromSource(
		"", actualPath,
		{{Graphics::EShaderStage::Vertex, "vertMain"}, {Graphics::EShaderStage::Fragment, "fragmentMain"}});

	HUSH_ASSERT(compilationResult.success, "Could not compile grid shader, diagnostics: {}!",
				compilationResult.diagnostics);

	const CompiledShaderStage *vertStage = compilationResult.FindStage(Graphics::EShaderStage::Vertex);
	const CompiledShaderStage *fragStage = compilationResult.FindStage(Graphics::EShaderStage::Fragment);

	this->m_vertModule = device->CreateShaderModule(vertStage->moduleDesc);
	this->m_fragModule = device->CreateShaderModule(fragStage->moduleDesc);

	std::vector<BindGroupLayoutDescriptor> layouts = compilationResult.BuildBindGroupLayoutDescriptors();
	// Set index
	this->m_gridBindGroupLayout = device->CreateBindGroupLayout(layouts[0]);

	// View uniform buffer
	this->m_gridUniformBuffer = device->CreateBuffer(
		{.size = sizeof(GridViewUniforms), .usage = EBufferUsage::Uniform, .memoryAccess = EMemoryAccess::CPUNone});

	GraphicsPipelineDescriptor desc{};
	desc.vertexStage = {this->m_vertModule.get()};
	desc.fragmentStage = {this->m_fragModule.get()};
	desc.primitive.topology = EPrimitiveTopology::TriangleList;
	desc.colorTargets = {
		{.format = ETextureFormat::BGRA8_UNORM,
		 .blendEnabled = true,
		 .colorBlend = {.srcFactor = EBlendFactor::SrcAlpha, .dstFactor = EBlendFactor::OneMinusSrcAlpha},
		 .alphaBlend = {.srcFactor = EBlendFactor::SrcAlpha, .dstFactor = EBlendFactor::OneMinusSrcAlpha}}};
	desc.bindGroupLayouts[0] = this->m_gridBindGroupLayout.get();
	desc.bindGroupLayoutCount = 1;
	// Compatible with the scene pass depth attachment — grid doesn't write to depth
	desc.depthStencil = {.enabled = true,
						 .format = ETextureFormat::D32_FLOAT,
						 .depthWriteEnabled = false,
						 .depthCompare = ECompareFunction::Always};
	this->m_gridPipeline = device->CreateGraphicsPipeline(desc);

	BindGroupDescriptor bgDesc{};
	bgDesc.layout = this->m_gridBindGroupLayout.get();
	bgDesc.entries = {
		{.binding = 0, .buffer = m_gridUniformBuffer.get(), .offset = 0, .size = sizeof(GridViewUniforms)}};
	this->m_gridBindGroup = device->CreateBindGroup(bgDesc);
}

Hush::Graphics::ShaderCompilationResult Hush::RenderingSystem::SetupMeshPipeline(
	Graphics::IGraphicsDevice *device, VirtualFilesystem *vfs, Graphics::ShaderCompiler *shaderCompiler)
{
	constexpr std::string_view meshVirtualPath = "engine_res://res/shaders/mesh.slang";
	auto meshRes = vfs->ResolveVirtualPath(meshVirtualPath);
	HUSH_RESULT_ASSERT(meshRes, "Could not load mesh shader! Make sure it's present on {}", meshVirtualPath);

	std::string_view meshActualPath = meshRes.value();
	Graphics::ShaderCompilationResult meshCompilationResult = shaderCompiler->CompileFromSource(
		"", meshActualPath,
		{{Graphics::EShaderStage::Vertex, "vertMain"}, {Graphics::EShaderStage::Fragment, "fragMain"}});

	HUSH_ASSERT(meshCompilationResult.success, "Could not compile mesh shader, diagnostics: {}!",
				meshCompilationResult.diagnostics);

	const CompiledShaderStage *meshVertStage = meshCompilationResult.FindStage(Graphics::EShaderStage::Vertex);
	const CompiledShaderStage *meshFragStage = meshCompilationResult.FindStage(Graphics::EShaderStage::Fragment);

	this->m_meshVertModule = device->CreateShaderModule(meshVertStage->moduleDesc);
	this->m_meshFragModule = device->CreateShaderModule(meshFragStage->moduleDesc);

	// ── Bind group layouts (auto-generated from reflection) ──────
	std::vector<BindGroupLayoutDescriptor> meshLayouts = meshCompilationResult.BuildBindGroupLayoutDescriptors();
	HUSH_ASSERT(meshLayouts.size() >= 1,
				"Mesh shader must have at least one bind group layout (set 0: scene + model)!");

	// Mark set 0 binding 1 (ModelData) as having dynamic offset
	for (auto &entry : meshLayouts[0].entries)
	{
		if (entry.binding == 1)
		{
			entry.hasDynamicOffset = true;
		}
	}

	this->m_meshSceneBindGroupLayout = device->CreateBindGroupLayout(meshLayouts[0]);
	if (meshLayouts.size() >= 2)
	{
		this->m_meshMaterialBindGroupLayout = device->CreateBindGroupLayout(meshLayouts[1]);
	}

	// Default placeholder textures
	{
		auto create1x1Texture = [device](std::unique_ptr<IGraphicsTexture> &outTex, const uint8_t rgba[4],
										 const char *debugName) {
			auto staging = device->CreateBuffer({.size = 4,
												 .usage = EBufferUsage::CopySource,
												 .memoryAccess = EMemoryAccess::CPUWrite,
												 .debugName = debugName});

			void *mapped = staging->Map(device);
			std::memcpy(mapped, rgba, 4);
			staging->Unmap();

			outTex = device->CreateTexture({.width = 1,
											.height = 1,
											.format = ETextureFormat::RGBA8_UNORM,
											.usage = ETextureUsage::Sampled | ETextureUsage::CopyDestination,
											.debugName = debugName});

			auto copyCmd = device->CreateCopyCommandList();
			copyCmd->CopyBufferToTexture(staging.get(), 0, outTex.get(), 0, 0, 0, 1, 1, 1, 256);
			copyCmd->Close();
			ICommandList *cmdRaw = copyCmd.get();
			device->GetGraphicsQueue()->Submit(std::span<ICommandList *>{&cmdRaw, 1});
			device->GetGraphicsQueue()->WaitIdle();
		};

		uint8_t white[] = {255, 255, 255, 255};
		uint8_t flatNormal[] = {128, 128, 255, 255};
		uint8_t black[] = {0, 0, 0, 255};
		uint8_t defaultMetalRough[] = {255, 128, 0, 255};

		// NOLINTBEGIN
		create1x1Texture(m_defaultColorTex, white, "DefaultColorTex");
		create1x1Texture(m_defaultMetalRoughTex, defaultMetalRough, "DefaultMetalRoughTex");
		create1x1Texture(m_defaultNormalTex, flatNormal, "DefaultNormalTex");
		create1x1Texture(m_defaultEmissiveTex, black, "DefaultEmissiveTex");
		// NOLINTEND
	}

	// Default sampler
	{
		SamplerDescriptor samplerDesc{};
		samplerDesc.debugName = "DefaultMeshSampler";
		m_defaultSampler = device->CreateSampler(samplerDesc);
	}

	// Graphics Pipeline
	{
		GraphicsPipelineDescriptor meshDesc{};
		meshDesc.vertexStage = {this->m_meshVertModule.get()};
		meshDesc.fragmentStage = {this->m_meshFragModule.get()};

		// Match to VertexInput struct on the shader
		meshDesc.vertexBufferLayouts.push_back(VertexBufferLayout{
			.stride = sizeof(Mesh::Vertex),
			.stepMode = EVertexStepMode::Vertex,
			.attributes = {
				{.shaderLocation = 0, .offset = offsetof(Mesh::Vertex, position), .format = EVertexFormat::Float32x3},
				{.shaderLocation = 1, .offset = offsetof(Mesh::Vertex, normal), .format = EVertexFormat::Float32x3},
				{.shaderLocation = 2, .offset = offsetof(Mesh::Vertex, color), .format = EVertexFormat::Float32x4},
				{.shaderLocation = 3, .offset = offsetof(Mesh::Vertex, tangent), .format = EVertexFormat::Float32x4},
				{.shaderLocation = 4, .offset = offsetof(Mesh::Vertex, uv), .format = EVertexFormat::Float32x2}}});

		meshDesc.primitive.topology = EPrimitiveTopology::TriangleList;
		meshDesc.primitive.cullMode = ECullModeFlags::Back;
		meshDesc.primitive.frontFace = EFrontFace::CounterClockwise;

		meshDesc.colorTargets = {{.format = ETextureFormat::BGRA8_UNORM}};

		meshDesc.depthStencil = {.enabled = true,
								 .format = ETextureFormat::D32_FLOAT,
								 .depthWriteEnabled = true,
								 .depthCompare = ECompareFunction::Less};

		meshDesc.bindGroupLayouts[0] = this->m_meshSceneBindGroupLayout.get();
		if (this->m_meshMaterialBindGroupLayout != nullptr)
		{
			meshDesc.bindGroupLayouts[1] = this->m_meshMaterialBindGroupLayout.get();
			meshDesc.bindGroupLayoutCount = 2;
		}
		else
		{
			meshDesc.bindGroupLayoutCount = 1;
		}

		meshDesc.debugName = "MeshPipeline";
		this->m_meshPipeline = device->CreateGraphicsPipeline(meshDesc);
	}

	// Uniform buffers
	constexpr uint32_t minOffsetAlignment = 256;
	uint32_t meshModelSlotSize = (sizeof(ModelData) + minOffsetAlignment - 1) & ~(minOffsetAlignment - 1);
	this->m_meshModelSlotSize = meshModelSlotSize;

	// If we exceed the count of mesh instances here we need to call device->ResizeBuffer()
	constexpr uint32_t initialMeshInstancePoolSize = 1024;
	this->m_meshModelBuffer = device->CreateBuffer({.size = static_cast<uint64_t>(meshModelSlotSize) * initialMeshInstancePoolSize,
													.usage = EBufferUsage::Uniform,
													.memoryAccess = EMemoryAccess::CPUNone,
													.debugName = "MeshModelBuffer"});

	this->m_sceneDataBuffer = device->CreateBuffer({.size = sizeof(SceneData),
													.usage = EBufferUsage::Uniform,
													.memoryAccess = EMemoryAccess::CPUNone,
													.debugName = "SceneDataBuffer"});

	// This assumes the material will always be the default PBR, which is fine for a general buffer, but, we will need per material buffers
	this->m_meshMaterialBuffer = device->CreateBuffer({.size = sizeof(PBRMaterialData),
													   .usage = EBufferUsage::Uniform,
													   .memoryAccess = EMemoryAccess::CPUNone,
													   .debugName = "MeshMaterialBuffer"});

	// Bind groups
	{
		BindGroupDescriptor sceneBgDesc{};
		sceneBgDesc.layout = this->m_meshSceneBindGroupLayout.get();
		sceneBgDesc.entries = {
			{.binding = 0, .buffer = this->m_sceneDataBuffer.get(), .offset = 0, .size = sizeof(SceneData)},
			{.binding = 1, .buffer = this->m_meshModelBuffer.get(), .offset = 0, .size = meshModelSlotSize}};
		this->m_meshSceneBindGroup = device->CreateBindGroup(sceneBgDesc);
	}

	if (this->m_meshMaterialBindGroupLayout != nullptr)
	{
		BindGroupDescriptor matBgDesc{};
		matBgDesc.layout = this->m_meshMaterialBindGroupLayout.get();
		matBgDesc.entries = {
			{.binding = 0, .buffer = this->m_meshMaterialBuffer.get(), .offset = 0, .size = sizeof(PBRMaterialData)},
			{.binding = 1, .texture = this->m_defaultColorTex.get()},
			{.binding = 2, .texture = this->m_defaultMetalRoughTex.get()},
			{.binding = 3, .texture = this->m_defaultNormalTex.get()},
			{.binding = 4, .texture = this->m_defaultEmissiveTex.get()},
			{.binding = 5, .sampler = this->m_defaultSampler.get()}};
		this->m_meshMaterialBindGroup = device->CreateBindGroup(matBgDesc);
	}

	PBRMaterialData defaultMaterial{};
	defaultMaterial.colorFactors = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
	defaultMaterial.metalRoughFactors = glm::vec4(1.0f, 0.5f, 0.0f, 0.0f);
	defaultMaterial.emissionFactors = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
	defaultMaterial.alphaCutoff = 0.5f;
	defaultMaterial.optionFlags = EPBRMaterialFlags::None;
	device->WriteBuffer(this->m_meshMaterialBuffer.get(), 0, &defaultMaterial, sizeof(PBRMaterialData));
	return meshCompilationResult;
}
