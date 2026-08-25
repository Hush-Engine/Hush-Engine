#include "RenderingSystem.hpp"
#include "Assertions.hpp"
#include "Components/ComponentMetadata.hpp"
#include "Components/GlobalKeys.hpp"
#include "Components/GpuUploadComponent.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/Material3D.hpp"
#include "Components/MeshReference.hpp"
#include "Components/RenderGraphBuilderComponent.hpp"
#include "Components/Serializable.hpp"
#include "Components/WorldTransform.hpp"
#include "Entity.hpp"
#include "HushEngine.hpp"
#include "Loaders/CrossLoaderDefinitions.hpp"
#include "Loaders/HMeshLoader.hpp"
#include "Logger.hpp"
#include "NullTerminatedStringView.hpp"
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
#include "Ref.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "ResourceManager.hpp"
#include "Scene.hpp"
#include "Shared/Camera.hpp"
#include "Shared/DirectionalLight.hpp"
#include "Shared/EditorCamera.hpp"
#include "Shared/PBRMaterial.hpp"
#include "Systems/RenderingSystemAPI.hpp"
#include "Vector4Math.hpp"
#include "VirtualFilesystem.hpp"
#include "WindowManager.hpp"
#include "crypto/Hashing.hpp"
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/quaternion_common.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/matrix.hpp>
#include <cstddef>
#include <cstring>
#include <span>
#include <string>
#include <string_view>

#include "StringAllocation.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"

using namespace Hush::Graphics;

// HACK: hacky function, see its implementation for details
Hush::Entity::EntityId InstantiateMeshEntities(const char* virtualPath, void* instance);

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

void Hush::RenderingSystem::BuildGameViewPassFunction(Hush::RenderGraph::RenderGraph &graph,
													  Hush::RenderingSystem *self)
{
	using namespace Hush::Graphics;
	using namespace Hush::RenderGraph;

	using RenderGraphBuildContext_t = Hush::RenderGraph::RenderGraph::BuildContext;

	struct GameViewPassData
	{
		ResourceId renderTexture;
		ResourceId depthTexture;
	};

	graph.AddPass<GameViewPassData>(
		EPassType::Graphics, "GameViewPass",
		[self](RenderGraphBuildContext_t &ctx, GameViewPassData &data) {
			const glm::u32vec2 bufferSize = self->m_cachedGameViewportSize;

			data.renderTexture = ctx.Create<TextureResource>(
				"GameViewPass_RenderTexture", TextureDescriptor{
												  .width = bufferSize.x,
												  .height = bufferSize.y,
												  .format = ETextureFormat::BGRA8_UNORM,
												  .usage = ETextureUsage::RenderTarget | ETextureUsage::Sampled,
											  });

			data.depthTexture = ctx.Create<TextureResource>("GameViewPass_DepthTexture",
															TextureDescriptor{.width = bufferSize.x,
																			  .height = bufferSize.y,
																			  .format = ETextureFormat::D32_FLOAT,
																			  .usage = ETextureUsage::DepthStencil});

			ctx.SetCullingMode(RenderPassNode::EPassCullingMode::NeverCull);
		},
		[self](GameViewPassData &data, ICommandList *cmdList,
			   const Hush::RenderGraph::ResourceManager &resourceManager) {
			auto *cmd = dynamic_cast<IGraphicsCommandList *>(cmdList);
			if (cmd == nullptr)
			{
				Hush::LogFormat(Hush::ELogLevel::Error, "[GameViewPass] Failed to get graphics command list.");
				return;
			}

			RenderPassDescriptor renderPass{};
			renderPass.debugLabel = "GameViewPass";

			RenderPassColorAttachment colorAttachment{};
			colorAttachment.texture = resourceManager.GetResource<TextureResource>(data.renderTexture)->texture.get();
			colorAttachment.loadOp = ELoadOp::Clear;
			colorAttachment.storeOp = EStoreOp::Store;
			colorAttachment.clearValue = ClearColorValue{0.0f, 0.0f, 0.0f, 1.0f};
			renderPass.AddColorAttachment(colorAttachment);

			RenderPassDepthStencilAttachment depthAttachment{};
			depthAttachment.texture = resourceManager.GetResource<TextureResource>(data.depthTexture)->texture.get();
			depthAttachment.depthLoadOp = ELoadOp::Clear;
			depthAttachment.depthStoreOp = EStoreOp::Store;
			depthAttachment.depthClearValue = 1.0f;
			renderPass.SetDepthStencilAttachment(depthAttachment);

			cmd->BeginRenderPass(renderPass);

			if (self->m_hasValidGameCamera)
			{
				cmd->SetViewport(0, 0, static_cast<float>(self->m_cachedGameViewportSize.x),
								 static_cast<float>(self->m_cachedGameViewportSize.y), 0, 1);

				auto *graphicsDevice = WindowManager::GetMainWindow()->GetGraphicsDevice();
				graphicsDevice->WriteBuffer(self->m_gameSceneDataBuffer.get(), 0, &self->m_cachedGameSceneData,
											sizeof(SceneData));

				// The editor grid is an editing aid, the game view only renders scene meshes
				cmd->BindPipeline(self->m_meshPipeline.get());

				for (const auto &draw : self->m_meshDrawList)
				{
					cmd->SetVertexBuffer(0, draw.vertexBuffer);
					cmd->SetIndexBuffer(draw.indexBuffer);
					cmd->SetBindGroup(0, self->m_gameMeshSceneBindGroup.get(),
									  std::span<const uint32_t>{&draw.dynamicOffset, 1});
					if (draw.materialBindGroup != nullptr)
					{
						cmd->SetBindGroup(1, draw.materialBindGroup);
					}
					cmd->DrawIndexed(draw.indexCount, 1, draw.firstIndex, 0, 0);
				}
			}

			cmd->EndRenderPass();
		});
}

Hush::Serializable::EError MeshReferenceDeserialize(uint8_t *self, Hush::Serialization::JsonDeserializer &serializer,
													void *ctx)
{
	using namespace Hush;
	// Raw deserialize to get the resourceId
	Serializable::DefaultDeserialize<MeshReference>(self, serializer);
	auto *instance = reinterpret_cast<MeshReference *>(self);
	auto *renderCtx = reinterpret_cast<RenderingContext *>(ctx);
	Scene *scene = renderCtx->activeScene;

	uint32_t resourceId = instance->GetResourceId();
	ResourceManager *resourceManager = scene->GetEngine()->GetResourceManager();

	Ref<Mesh> existingMesh = resourceManager->GetRefOrNull<Mesh>(resourceId);

	// Then we check if there exists any Ref<Mesh> with this identifier
	if (!existingMesh.IsNull())
	{
		instance->SetMesh(existingMesh);
		// Also set the material refs
		for (uint32_t materialId : instance->GetMaterialIds())
		{
			Ref<Material3D> mat = resourceManager->GetRefOrNull<Material3D>(materialId);
			HUSH_ASSERT(!mat.IsNull(), "Material {} was not properly created in the first step of serialization",
						materialId);
			instance->PushMaterial(mat);
		}
		return Serializable::EError::None;
	}

	{
		// Do not keep this one around
		auto mesh = resourceManager->AllocateRefKnwonID<Mesh>(resourceId);
		instance->SetMesh(mesh);
	}

	// If it does not exist, we read the asset file and create the mesh references
	// This is the slow path but it'll run only once per mesh
	VirtualFilesystem *vfs = scene->GetEngine()->GetVirtualFilesystem();
	auto openRes = vfs->OpenFile(std::string("res://.hcooked/") + std::to_string(resourceId) + ".hasset");

	if (openRes.has_error())
	{
		return Serializable::EError::MissingInternalResource;
	}

	// Pass this to the hush mesh parser
	std::pmr::memory_resource *allocator = scene->GetEngine()->GetFrameScopeMemoryResource();
	const size_t meshFileSize = openRes.value()->GetFileInfo().size;
	auto *meshFileBuffer = reinterpret_cast<std::byte *>(allocator->allocate(meshFileSize));
	auto buffer = std::span<std::byte>{meshFileBuffer, meshFileSize};
	auto readFileRes = openRes.value()->Read(buffer);
	if (readFileRes.has_error())
	{
		return Serializable::EError::MissingInternalResource;
	}

	HMeshLoader::LoadMeshFromBinary(buffer, instance, renderCtx);

	allocator->deallocate(meshFileBuffer, meshFileSize, alignof(std::byte *));

	return Serializable::EError::None;
}

void MeshReferencePostDeserialize(uint8_t *instance, Hush::Entity::EntityId entity, Hush::Entity::EntityId comp,
								  void *ctx)
{
	using namespace Hush;
	(void)instance;
	(void)comp;
	// Add the GPU upload comp
	auto *renderCtx = reinterpret_cast<RenderingContext *>(ctx);
	Scene *scene = renderCtx->activeScene;
	Entity ent = scene->EntityFromIdUnchecked(entity);
	ent.AddComponent<Renderer::GpuUploadComponent>();
}

void Hush::RenderingSystem::Init()
{
	// Register our public rendering comps for editor inspection
	Entity::EntityId dirLightId = this->GetScene().RegisterComponent<DirectionalLight>();
	Entity dirLightComp = this->GetScene().EntityFromIdUnchecked(dirLightId);
	dirLightComp.AddComponent<InspectableComponent>();
	{
		this->GetScene().RegisterDefaultSerializer<DirectionalLight>();
	}

	Entity::EntityId meshRefId = this->GetScene().RegisterComponent<MeshReference>();
	Entity meshRefComp = this->GetScene().EntityFromIdUnchecked(meshRefId);
	meshRefComp.AddComponent<InspectableComponent>();
	{
		Serializable &ser = meshRefComp.AddComponent<Serializable>();
		ser.serialize = &Serializable::DefaultSerialize<MeshReference>;
		ser.deserialize = &::MeshReferenceDeserialize;
		ser.postDeserialize = &::MeshReferencePostDeserialize;
		ser.ctx = &this->m_renderingContext;
	}

	Entity::EntityId camRefId = this->GetScene().RegisterComponent<Camera>();
	Entity camRefComp = this->GetScene().EntityFromIdUnchecked(camRefId);
	camRefComp.AddComponent<InspectableComponent>();
	{
		Serializable &ser = camRefComp.AddComponent<Serializable>();
		ser.serialize = &Serializable::DefaultSerialize<Camera>;
		ser.deserialize = &Serializable::DefaultDeserialize<Camera>;
	}
	// Weird, but this is how flecs creates systems, they are associated with an entity and we can query for them
	Entity selfEntity = this->GetScene().CreateEntityWithKey("RenderingSystem");
	auto &renderingSystemRef = selfEntity.AddComponent<RenderingSystem *>();
	renderingSystemRef = this;
	auto &renderingSystemAPIRef = selfEntity.AddComponent<RenderingSystemAPI>();
	renderingSystemAPIRef.instantiateMeshEntities = &::InstantiateMeshEntities;
	renderingSystemAPIRef.instance = this;

	// Expose the scripting-facing API through the interface component so scripts
	// can query the RenderingSystemAPI component and call into the system.

	// TODO: Check why we can't do Cache::All
	this->m_renderableTargetsQuery =
		this->GetScene().CreateQuery<const MeshReference, const WorldTransform>(RawQuery::ECacheMode::Auto);

	this->GetScene().AddComponentObserver<ScenePanelSizeComp>(
		EComponentObserverType::Set, [this](Entity::EntityId entity, ScenePanelSizeComp *panelSize) {
			(void)entity;
			this->m_cachedViewportSize = panelSize->size;
		});

	this->m_editorCameraQuery = this->GetScene().CreateQuery<EditorCamera>();

	this->m_gameCameraQuery = this->GetScene().CreateQuery<Camera, WorldTransform>();

	this->m_directionalLightsQuery = this->GetScene().CreateQuery<DirectionalLight, WorldTransform>();

	// Make sure we have the data so that initialization order does not matter
	auto scenePanelQuery = this->GetScene().CreateQuery<const ScenePanelSizeComp>(RawQuery::ECacheMode::None);
	scenePanelQuery.Each([this](const ScenePanelSizeComp &sizeComp) { this->m_cachedViewportSize = sizeComp.size; });

	auto gamePanelQuery = this->GetScene().CreateQuery<const GamePanelSizeComp>(RawQuery::ECacheMode::None);
	gamePanelQuery.Each([this](const GamePanelSizeComp &sizeComp) { this->m_cachedGameViewportSize = sizeComp.size; });

	this->GetScene().AddComponentObserver<GamePanelSizeComp>(
		EComponentObserverType::Set, [this](Entity::EntityId entity, GamePanelSizeComp *panelSize) {
			(void)entity;
			this->m_cachedGameViewportSize = panelSize->size;
		});

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
	scenePassBuilder.builderFunc = [this](RenderGraph::RenderGraph &graph) {
		BuildScenePassFunction(graph, this);
		BuildGameViewPassFunction(graph, this);
	};
	scenePassBuilder.frameUpdateFunc = [](Hush::RenderGraph::RenderGraph &) {};

	this->m_pbrMaterialDescriptor = {
		.vertexShader = this->m_meshVertModule.get(),
		.fragmentShader = this->m_meshFragModule.get(),
		.vertexEntry = "vertMain",
		.fragmentEntry = "fragMain",
		.compilationResult = &this->m_pbrCompilationData,
		.colorTargetFormat = ETextureFormat::BGRA8_UNORM,
	};

	HushEngine *engine = this->GetScene().GetEngine();
	this->m_renderingContext.virtualFilesystem = engine->GetVirtualFilesystem();
	this->m_renderingContext.activeScene = &this->GetScene();
	this->m_renderingContext.device = engine->GetWindowRenderer()->GetGraphicsDevice();
	this->m_renderingContext.materialDescriptor = &this->m_pbrMaterialDescriptor;
	this->m_renderingContext.resourceManager = engine->GetResourceManager();
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

	// This is a query because we'll want to support multiple dir lights later
	this->m_directionalLightsQuery.Each([this](Entity::EntityId, DirectionalLight &light, WorldTransform &xform) {
		this->m_cachedSceneData.sunlightColor = light.color.GetRGBA32F();
		this->m_cachedSceneData.sunlightColor.w = light.intensity;
		glm::vec3 dir = xform.Forward();
		this->m_cachedSceneData.sunlightDirection = glm::vec4(dir.x, dir.y, dir.z, light.intensity);
	});

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
	});

	// Game view data comes from entities with a Camera component, NOT from the editor camera.
	// The view matrix is the inverse of the entity's world transform (the camera looks down its forward axis).
	this->m_hasValidGameCamera = false;
	this->m_gameCameraQuery.Each([this](Entity::EntityId, Camera &cam, WorldTransform &xform) {
		cam.SetViewportSize((float)(this->m_cachedGameViewportSize.x), (float)(this->m_cachedGameViewportSize.y));
		glm::mat4 view = glm::inverse(xform.GetTransformationMatrix());
		glm::mat4 proj = cam.GetProjectionMatrix();

		this->m_cachedGameSceneData.view = view;
		this->m_cachedGameSceneData.proj = proj;
		this->m_cachedGameSceneData.viewproj = proj * view;
		this->m_cachedGameSceneData.ambientColor = glm::vec4(0.1f, 0.1f, 0.15f, 1.0f);
		this->m_cachedGameSceneData.sunlightDirection = this->m_cachedSceneData.sunlightDirection;
		this->m_cachedGameSceneData.sunlightColor = this->m_cachedSceneData.sunlightColor;
		this->m_hasValidGameCamera = true;
	});

	m_meshDrawList.clear();

	HUSH_ASSERT(this->m_meshPipeline != nullptr, "Mesh pipeline should not ever be null!");

	Graphics::IGraphicsDevice *device = WindowManager::GetMainWindow()->GetGraphicsDevice();
	uint32_t slotIndex = 0;

	size_t meshCount = this->m_renderableTargetsQuery.begin().Size();

	// Resize our mesh buffer if we get to the max amount of meshes
	uint64_t currBufferSize = this->m_meshModelBuffer->GetSize();
	constexpr float meshBufferGrowthFactor = 1.2f;
	if (meshCount > (currBufferSize / this->m_meshModelSlotSize))
	{
		auto resizeBy = (uint64_t)((float)(meshCount * this->m_meshModelSlotSize) * meshBufferGrowthFactor);
		device->ResizeBuffer(this->m_meshModelBuffer.get(), resizeBy);
		this->CreateMeshSceneBindGroup(device);
	}

	ResourceManager *resourceManager = this->GetScene().GetEngine()->GetResourceManager();

	m_renderableTargetsQuery.Each(
		[this, device, &slotIndex, resourceManager](const MeshReference &meshRef, const WorldTransform &xform) {
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

				auto *mat = resourceManager->GetRefOrNull<Material3D>(surface.materialResource).Get();
				if (mat != nullptr)
				{
					mat->FlushProperties(device);

					// Map known PBR bindings to their fallback defaults. Used both
					// when building a bind group (null slot -> default) and when
					// validating the cache, so the cached snapshot and the current
					// material state compare consistently. Comparing the raw slot
					// pointer (nullptr for unbound slots) against the cached resolved
					// default would spuriously invalidate the entry on EVERY surface,
					// freeing a bind group that an already-built draw still
					// references (a deterministic dangling-pointer crash once a mesh
					// has more than one surface).
					auto defaultTextureForBinding = [&](uint32_t binding) -> IGraphicsTexture * {
						switch (binding)
						{
						case 1:
							return m_defaultColorTex.get();
						case 2:
							return m_defaultMetalRoughTex.get();
						case 3:
							return m_defaultNormalTex.get();
						case 4:
							return m_defaultEmissiveTex.get();
						default:
							return m_defaultColorTex.get();
						}
					};

					auto it = m_materialBindGroupCache.find(mat);
					if (it != m_materialBindGroupCache.end())
					{
						// Validate cache: check if any texture pointer changed (async upload completion).
						bool cacheValid = true;
						const auto &slots = mat->GetTextureSlots();
						for (const auto &slot : slots)
						{
							IGraphicsTexture *currentTex =
								slot.texture != nullptr ? slot.texture : defaultTextureForBinding(slot.binding);
							auto cachedTex = it->second.textures.find(slot.binding);
							if (cachedTex == it->second.textures.end())
							{
								if (currentTex != nullptr)
								{
									cacheValid = false;
									break;
								}
							}
							else if (cachedTex->second != currentTex)
							{
								cacheValid = false;
								break;
							}
						}
						if (!cacheValid)
						{
							m_materialBindGroupCache.erase(it);
							it = m_materialBindGroupCache.end();
						}
					}

					if (it == m_materialBindGroupCache.end())
					{
						BindGroupDescriptor bgDesc{};
						bgDesc.layout = m_meshMaterialBindGroupLayout.get();
						bgDesc.debugName = mat->GetName() + "_BindGroup";
						uint64_t bufSize = mat->GetUniformBufferSize();
						bgDesc.entries.push_back(
							{.binding = 0, .buffer = mat->GetUniformBuffer(), .offset = 0, .size = bufSize});

						CachedMaterialBindGroup cachedEntry;

						const auto &slots = mat->GetTextureSlots();
						for (const auto &slot : slots)
						{
							IGraphicsTexture *tex =
								slot.texture != nullptr ? slot.texture : defaultTextureForBinding(slot.binding);
							bgDesc.entries.push_back({.binding = slot.binding, .texture = tex});
							cachedEntry.textures[slot.binding] = tex;
						}

						bgDesc.entries.push_back({.binding = 5, .sampler = m_defaultSampler.get()});

						cachedEntry.bindGroup = device->CreateBindGroup(bgDesc);
						auto [newIt, _] = m_materialBindGroupCache.emplace(mat, std::move(cachedEntry));
						it = newIt;
					}
					matBindGroup = it->second.bindGroup.get();
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

// HACK: Utility for the user side API, this is, in fact, quite bad but it's because this
// class should NOT belong to the editor at all
Hush::Entity::EntityId InstantiateMeshEntities(const char* virtualPath, void* instance)
{
	using namespace Hush;
	auto* self = reinterpret_cast<RenderingSystem*>(instance);
	Scene &scene = self->GetScene();
	ResourceManager *resourceManager = scene.GetEngine()->GetResourceManager();
	VirtualFilesystem *vfs = scene.GetEngine()->GetVirtualFilesystem();

	Entity entity = scene.CreateEntityWithName("RuntimeMesh");
	entity.AddComponent<WorldTransform>();
	entity.AddComponent<LocalTransform>();

	// The mesh is keyed by the asset path so every instance of the same asset
	// shares one Mesh (and later one set of GPU buffers).
	const uint32_t resourceId = Hashing::Fnv1a(virtualPath);
	Ref<Mesh> mesh = resourceManager->AllocateRefKnwonID<Mesh>(resourceId);
	auto &meshRefComp = entity.EmplaceComponent<MeshReference>(mesh);
	meshRefComp.SetResourcePath(virtualPath, "RuntimeMesh");

	// Parse the cooked HAsset only once; later instances reuse the shared Mesh.
	if (mesh->GetVertexBuffer().empty())
	{
		auto openRes = vfs->OpenFile(virtualPath);
		HUSH_ASSERT(!openRes.has_error(), "InstantiateMeshEntities: cannot open '{}' (is the asset cooked?)",
					virtualPath);
		if (openRes.has_error())
		{
			scene.DestroyEntity(entity);
			return Entity::INVALID_ENTITY_ID;
		}

		const size_t fileSize = openRes.value()->GetFileInfo().size;
		std::vector<std::byte> buffer(fileSize);
		auto readRes = openRes.value()->Read(buffer);
		HUSH_ASSERT(!readRes.has_error(), "InstantiateMeshEntities: failed to read '{}'", virtualPath);
		if (readRes.has_error())
		{
			scene.DestroyEntity(entity);
			return Entity::INVALID_ENTITY_ID;
		}

		const bool loaded = HMeshLoader::LoadMeshFromBinary(buffer, &meshRefComp, self->GetRenderingContext());
		HUSH_ASSERT(loaded, "InstantiateMeshEntities: '{}' is not a valid cooked mesh asset", virtualPath);
		if (!loaded)
		{
			scene.DestroyEntity(entity);
			return Entity::INVALID_ENTITY_ID;
		}
	}

	// Let ResourceUploadSystem create + upload the GPU vertex/index buffers.
	entity.AddComponent<Renderer::GpuUploadComponent>();

	return entity.GetId();
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

	NullTerminatedStringView actualPath =
		MakeNullTerminated(res.value(), GetScene().GetEngine()->GetFrameScopeMemoryResource());

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

	NullTerminatedStringView meshActualPath =
		MakeNullTerminated(meshRes.value(), GetScene().GetEngine()->GetFrameScopeMemoryResource());
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
	this->m_meshModelBuffer =
		device->CreateBuffer({.size = static_cast<uint64_t>(meshModelSlotSize) * initialMeshInstancePoolSize,
							  .usage = EBufferUsage::Uniform,
							  .memoryAccess = EMemoryAccess::CPUNone,
							  .debugName = "MeshModelBuffer"});

	this->m_sceneDataBuffer = device->CreateBuffer({.size = sizeof(SceneData),
													.usage = EBufferUsage::Uniform,
													.memoryAccess = EMemoryAccess::CPUNone,
													.debugName = "SceneDataBuffer"});

	// The game view has its own scene data buffer so both cameras can be uploaded on the same frame
	this->m_gameSceneDataBuffer = device->CreateBuffer({.size = sizeof(SceneData),
														.usage = EBufferUsage::Uniform,
														.memoryAccess = EMemoryAccess::CPUNone,
														.debugName = "GameSceneDataBuffer"});

	// This assumes the material will always be the default PBR, which is fine for a general buffer, but, we will need
	// per material buffers
	this->m_meshMaterialBuffer = device->CreateBuffer({.size = sizeof(PBRMaterialData),
													   .usage = EBufferUsage::Uniform,
													   .memoryAccess = EMemoryAccess::CPUNone,
													   .debugName = "MeshMaterialBuffer"});

	// Bind groups
	{
		this->CreateMeshSceneBindGroup(device);
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

void Hush::RenderingSystem::CreateMeshSceneBindGroup(Graphics::IGraphicsDevice *device)
{
	BindGroupDescriptor sceneBgDesc{};
	sceneBgDesc.layout = this->m_meshSceneBindGroupLayout.get();
	sceneBgDesc.entries = {
		{.binding = 0, .buffer = this->m_sceneDataBuffer.get(), .offset = 0, .size = sizeof(SceneData)},
		{.binding = 1, .buffer = this->m_meshModelBuffer.get(), .offset = 0, .size = this->m_meshModelSlotSize}};
	this->m_meshSceneBindGroup = device->CreateBindGroup(sceneBgDesc);

	// Same model buffer as the editor scene pass, only the camera data differs
	BindGroupDescriptor gameSceneBgDesc{};
	gameSceneBgDesc.layout = this->m_meshSceneBindGroupLayout.get();
	gameSceneBgDesc.entries = {
		{.binding = 0, .buffer = this->m_gameSceneDataBuffer.get(), .offset = 0, .size = sizeof(SceneData)},
		{.binding = 1, .buffer = this->m_meshModelBuffer.get(), .offset = 0, .size = this->m_meshModelSlotSize}};
	this->m_gameMeshSceneBindGroup = device->CreateBindGroup(gameSceneBgDesc);
}
