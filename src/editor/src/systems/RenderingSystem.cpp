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
#include "RHI/ShaderCompiler.hpp"
#include "../components/EditorPanelComponents.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Scene.hpp"
#include "Shared/EditorCamera.hpp"
#include "VirtualFilesystem.hpp"
#include "WindowManager.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <string_view>

using namespace Hush::Graphics;

void Hush::RenderingSystem::BuildGridPassFunction(Hush::RenderGraph::RenderGraph& graph, Hush::RenderingSystem* self) {
	using namespace Hush::Graphics;
	using namespace Hush::RenderGraph;

	using RenderGraphBuldContext_t = Hush::RenderGraph::RenderGraph::BuildContext;

	struct GridPassData {
		ResourceId sceneTexture;
		ResourceId depthTexture;
	};

	graph.AddPass<GridPassData>(EPassType::Graphics, "GridPass", [self](RenderGraphBuldContext_t& ctx, GridPassData& data) {
		// Build
		data.sceneTexture = ctx.Read(ctx.GetResourceIdByName("EditorScenePass_RenderTexture"), EResourceState::RenderTarget);
		// Get the scene texture ??
		// Create the depth buffer
		data.depthTexture = ctx.Create<TextureResource>("GridPass_Depth", {
		    .width = self->m_cachedViewportSize.x,
		    .height = self->m_cachedViewportSize.y,
			.format = ETextureFormat::D24_UNORM,
			.usage = ETextureUsage::DepthStencil
		});
	    ctx.SetCullingMode(RenderPassNode::EPassCullingMode::NeverCull);
	},
	[self](GridPassData& data, ICommandList* cmdList, const Hush::RenderGraph::ResourceManager& resourceManager){
		LogFormat(ELogLevel::Info, "Executing grid pass");
		auto *cmd = dynamic_cast<IGraphicsCommandList*>(cmdList);
        auto *sceneTex = resourceManager.GetResource<TextureResource>(data.sceneTexture)->texture.get();
        auto *depthTex = resourceManager.GetResource<TextureResource>(data.depthTexture)->texture.get();
        RenderPassDescriptor rp{};
        rp.AddColorAttachment({.texture = sceneTex, .loadOp = ELoadOp::Load, .storeOp = EStoreOp::Store});
        RenderPassDepthStencilAttachment depth{};
        depth.texture = depthTex;
        depth.depthLoadOp = ELoadOp::Clear;
        depth.depthStoreOp = EStoreOp::Store;
        depth.depthClearValue = 1.0f;
        rp.SetDepthStencilAttachment(depth);

		auto* graphicsDevice = WindowManager::GetMainWindow()->GetGraphicsDevice();
		graphicsDevice->WriteBuffer(self->m_gridUniformBuffer.get(), 0, &self->m_cachedViewUniforms, sizeof(ViewUniforms));

        cmd->BeginRenderPass(rp);
        cmd->SetViewport(0, 0, static_cast<float>(self->m_cachedViewportSize.x), static_cast<float>(self->m_cachedViewportSize.y), 0, 1);
        cmd->BindPipeline(self->m_gridPipeline.get());
        cmd->SetBindGroup(0, self->m_gridBindGroup.get());
        cmd->Draw(6, 1, 0, 0);
        cmd->EndRenderPass();		
	});
}

void Hush::RenderingSystem::Init()
{
	// TODO: Check why we can't do Cache::All
	this->m_renderableTargetsQuery =
		this->GetScene().CreateQuery<const MeshReference, const WorldTransform>(RawQuery::ECacheMode::Auto);

	this->GetScene().AddComponentObserver<ScenePanelSizeComp>(EComponentObserverType::Set, [this](Entity::EntityId entity, ScenePanelSizeComp* panelSize){
		this->m_cachedViewportSize = panelSize->size;
	});

	this->m_editorCameraQuery = this->GetScene().CreateQuery<EditorCamera>();

	// Make sure we have the data so that initialization order does not matter
	auto scenePanelQuery = this->GetScene().CreateQuery<const ScenePanelSizeComp>(RawQuery::ECacheMode::None);
	scenePanelQuery.Each([this](const ScenePanelSizeComp& sizeComp) {
		this->m_cachedViewportSize = sizeComp.size;
	});


	// We need to load the shaders here
	// Access the filesystem
	Entity engineManager = this->GetScene().CreateEntityWithKey(ENGINE_MANAGER);
	VirtualFilesystem *vfs = engineManager.GetComponent<Hush::VirtualFilesystem>();
	HUSH_ASSERT(vfs != nullptr, "File system on engine manager can't be null, check initialization order!");

	// With the filesystem
	constexpr std::string_view virtualPath = "engine_res://res/shaders/grid.slang";
	auto res = vfs->ResolveVirtualPath(virtualPath);
	HUSH_RESULT_ASSERT(res, "Could not load grid shader! Make sure it's present on {}", virtualPath);

	std::string_view actualPath = res.value();

	auto *shaderCompiler = engineManager.GetComponent<Hush::Graphics::ShaderCompiler>();
	HUSH_ASSERT(shaderCompiler != nullptr,
				"Shader compiler on engine manager can't be null, check initialization order!");

	// TODO: use the actual dynamic graphics API here:
	Graphics::EShaderTarget target = Graphics::ShaderCompiler::GetTargetForAPI(Graphics::EGraphicsAPI::WebGPU);
	shaderCompiler->Initialize({.target = target});
	Graphics::ShaderCompilationResult compilationResult = shaderCompiler->CompileFromSource(
		"", actualPath,
		{{Graphics::EShaderStage::Vertex, "vertMain"}, {Graphics::EShaderStage::Fragment, "fragmentMain"}});

	HUSH_ASSERT(compilationResult.success, "Could not compile grid shader, diagnostics: {}!",
				compilationResult.diagnostics);

	Graphics::IGraphicsDevice *device = WindowManager::GetMainWindow()->GetGraphicsDevice();

	const CompiledShaderStage *vertStage = compilationResult.FindStage(Graphics::EShaderStage::Vertex);
	const CompiledShaderStage *fragStage = compilationResult.FindStage(Graphics::EShaderStage::Fragment);

	this->m_vertModule = device->CreateShaderModule(vertStage->moduleDesc);
	this->m_fragModule = device->CreateShaderModule(fragStage->moduleDesc);

	std::vector<BindGroupLayoutDescriptor> layouts = compilationResult.BuildBindGroupLayoutDescriptors();
	// Set index
	this->m_gridBindGroupLayout = device->CreateBindGroupLayout(layouts[0]);

	// View uniform buffer
	this->m_gridUniformBuffer = device->CreateBuffer(
		{.size = sizeof(ViewUniforms), .usage = EBufferUsage::Uniform, .memoryAccess = EMemoryAccess::CPUNone});

	GraphicsPipelineDescriptor desc{};
	desc.vertexStage = {this->m_vertModule.get()};
	desc.fragmentStage = {this->m_fragModule.get()};
	desc.primitive.topology = EPrimitiveTopology::TriangleList;
	desc.colorTargets = {{.format = ETextureFormat::BGRA8_UNORM}};
	desc.bindGroupLayouts[0] = this->m_gridBindGroupLayout.get();
	desc.bindGroupLayoutCount = 1;
	// depth test so grid draws correctly behind opaque objects
	desc.depthStencil = {.enabled = true, .format = ETextureFormat::D24_UNORM};
	this->m_gridPipeline = device->CreateGraphicsPipeline(desc);


	BindGroupDescriptor bgDesc{};
	bgDesc.layout = this->m_gridBindGroupLayout.get();
	bgDesc.entries = {{.binding = 0, .buffer = m_gridUniformBuffer.get(), .offset = 0, .size = sizeof(ViewUniforms)}};
	this->m_gridBindGroup = device->CreateBindGroup(bgDesc);	

	Entity builderEnt = this->GetScene().CreateEntityWithKey("GridRenderGraph");
	auto& builder = builderEnt.AddComponent<RenderGraph::RenderGraphBuilderComponent>();
	builder.builderFunc = [this](RenderGraph::RenderGraph& graph) {
		BuildGridPassFunction(graph, this);
	};
	builder.frameUpdateFunc = [](Hush::RenderGraph::RenderGraph&){};
}

void Hush::RenderingSystem::OnShutdown()
{
}

void Hush::RenderingSystem::OnUpdate(float delta)
{
}

void Hush::RenderingSystem::OnFixedUpdate(float delta)
{
}

void Hush::RenderingSystem::OnRender()
{
}

void Hush::RenderingSystem::OnPreRender()
{
	ZoneScoped;
	// TODO: Update camera view matrix and everything else in the scene data here
	//
	// 

	this->m_editorCameraQuery.Each([this](Entity::EntityId ent, EditorCamera& editorCam) {
	    glm::mat4 viewProj = editorCam.GetProjectionMatrix() * editorCam.GetViewMatrix();
		this->m_cachedViewUniforms.viewproj = viewProj;
		this->m_cachedViewUniforms.farPlane = editorCam.GetFarPlane();
		this->m_cachedViewUniforms.pos = editorCam.GetPosition();
	});
	

	IRenderer *renderer = WindowManager::GetMainWindow()->GetInternalRenderer();
	this->m_renderableTargetsQuery.Each([&renderer](Entity &_, const MeshReference &mesh, const WorldTransform &xform) {
	    ZoneScopedN("Renderable meshes push");
		renderer->PushMesh(&xform, mesh.GetMesh().Get());
	});
}

void Hush::RenderingSystem::OnPostRender()
{
}

std::string_view Hush::RenderingSystem::GetName() const
{
	return "RenderingSystem";
}
