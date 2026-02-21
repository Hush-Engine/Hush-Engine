#include "DefaultRenderer.hpp"
#include "RHI/GraphicsResources.hpp"
#include "RHI/RenderPass.hpp"
#include "RHI/ICommandList.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "Shared/DirectionalLight.hpp"

static const Hush::Graphics::DefaultRenderer::GeometryPassResources &AddGeometryPass(
	Hush::WindowRenderer &windowRenderer, Hush::RenderGraph::RenderGraph &graph, Hush::Scene *scene)
{
	using namespace Hush::Graphics;
	using namespace Hush::RenderGraph;
	using namespace Hush::Graphics::DefaultRenderer;

	const auto &gPass = graph.AddPass<GeometryPassResources>(
		EPassType::Graphics, "GeometryPass",
		[&windowRenderer](RenderGraph::BuildContext &ctx, GeometryPassResources &data) {
		    const auto [width, height] = windowRenderer.GetWindowSize();
			// Create G-buffer textures
			data.geometryBufferAlbedo =
				ctx.Create<TextureResource>("GBuffer_Albedo", TextureDescriptor{
																  .width = width,
																  .height = height,
																  .format = ETextureFormat::RGBA8_UNORM,
																  .usage = ETextureUsage::RenderTarget,
															  });
			data.geometryBufferNormal =
				ctx.Create<TextureResource>("GBuffer_Normal", TextureDescriptor{
																  .width = width,
																  .height = height,
																  .format = ETextureFormat::RGBA8_UNORM,
																  .usage = ETextureUsage::RenderTarget,
															  });
			data.geometryDepthBuffer =
				ctx.Create<TextureResource>("GBuffer_Depth", TextureDescriptor{
																 .width = width,
																 .height = height,
																 .format = ETextureFormat::D24_UNORM_S8_UINT,
																 .usage = ETextureUsage::DepthStencil,
															 });
		},
		[scene](GeometryPassResources &data, Hush::Graphics::ICommandList *cmdList,
		   const Hush::RenderGraph::ResourceManager &resourceManager) {
			// Get the G-buffer textures
			auto *albedoTexture =
				resourceManager.GetResource<TextureResource>(data.geometryBufferAlbedo)->texture.get();
			auto *normalTexture =
				resourceManager.GetResource<TextureResource>(data.geometryBufferNormal)->texture.get();
			auto *depthTexture = resourceManager.GetResource<TextureResource>(data.geometryDepthBuffer)->texture.get();

			// Set up render pass descriptor
			RenderPassDescriptor renderPassDesc{};
			renderPassDesc.colorAttachments[0].texture = albedoTexture;
			renderPassDesc.colorAttachments[0].loadOp = ELoadOp::Clear;
			renderPassDesc.colorAttachments[0].storeOp = EStoreOp::Store;
			renderPassDesc.colorAttachments[0].clearValue = {0.0f, 0.0f, 0.0f, 1.0f};

			renderPassDesc.colorAttachments[1].texture = normalTexture;
			renderPassDesc.colorAttachments[1].loadOp = ELoadOp::Clear;
			renderPassDesc.colorAttachments[1].storeOp = EStoreOp::Store;
			renderPassDesc.colorAttachments[1].clearValue = {0.5f, 0.5f, 1.0f, 1.0f};

			renderPassDesc.depthStencilAttachment = RenderPassDepthStencilAttachment{
				.texture = depthTexture,

				.depthLoadOp = ELoadOp::Clear,
				.depthStoreOp = EStoreOp::Store,
				.depthClearValue = 1.0f,

				.stencilLoadOp = ELoadOp::Clear,
				.stencilStoreOp = EStoreOp::Store,
				.stencilClearValue = 0,
			};

			// Begin the render pass
			auto *renderCmdList =
				dynamic_cast<IGraphicsCommandList *>(cmdList); // NOLINT(*-pro-type-static-cast-downcast)

			renderCmdList->BeginRenderPass(renderPassDesc);

			auto meshQuery = scene->CreateQuery<const Hush::MeshReference, const Hush::WorldTransform>();
			for (const auto &[meshRef, transform] : meshQuery)
            {

            }

            renderCmdList->EndRenderPass();

		});

	return gPass;
}

static const Hush::Graphics::DefaultRenderer::LightingPassResources &AddLightingPass(
	Hush::WindowRenderer &windowRenderer, Hush::RenderGraph::RenderGraph &graph,
	const Hush::Graphics::DefaultRenderer::GeometryPassResources &geometryResources, [[maybe_unused]] Hush::Scene *scene)
{
	using namespace Hush::Graphics;
	using namespace Hush::RenderGraph;
	using namespace Hush::Graphics::DefaultRenderer;

	const auto &lightingPass = graph.AddPass<LightingPassResources>(
		EPassType::Graphics, "LightingPass",
		[&windowRenderer, &geometryResources](RenderGraph::BuildContext &ctx, LightingPassResources &data) {
			const auto [width, height] = windowRenderer.GetWindowSize();

			// Read G-buffer textures produced by the geometry pass
			ctx.Read(geometryResources.geometryBufferAlbedo, EResourceState::PixelShaderAccess);
			ctx.Read(geometryResources.geometryBufferNormal, EResourceState::PixelShaderAccess);
			ctx.Read(geometryResources.geometryDepthBuffer, EResourceState::DepthStencilRead);

			// Create the lighting output buffer
			data.lightingBuffer =
				ctx.Create<TextureResource>("LightingBuffer", TextureDescriptor{
																  .width = width,
																  .height = height,
																  .format = ETextureFormat::RGBA16_FLOAT,
																  .usage = ETextureUsage::RenderTarget,
															  });
		},
		[&geometryResources](LightingPassResources &data, Hush::Graphics::ICommandList *cmdList,
							 const Hush::RenderGraph::ResourceManager &resourceManager) {
			// Resolve G-buffer textures for reading
			[[maybe_unused]] auto *albedoTexture =
				resourceManager.GetResource<TextureResource>(geometryResources.geometryBufferAlbedo)->texture.get();
			[[maybe_unused]] auto *normalTexture =
				resourceManager.GetResource<TextureResource>(geometryResources.geometryBufferNormal)->texture.get();
			[[maybe_unused]] auto *depthTexture =
				resourceManager.GetResource<TextureResource>(geometryResources.geometryDepthBuffer)->texture.get();

			// Resolve the lighting output target
			auto *lightingTexture =
				resourceManager.GetResource<TextureResource>(data.lightingBuffer)->texture.get();

			// Set up render pass targeting the lighting buffer
			RenderPassDescriptor renderPassDesc{};
			renderPassDesc.debugLabel = "LightingPass";

			RenderPassColorAttachment colorAttachment{};
			colorAttachment.texture = lightingTexture;
			colorAttachment.loadOp = ELoadOp::Clear;
			colorAttachment.storeOp = EStoreOp::Store;
			colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
			renderPassDesc.AddColorAttachment(colorAttachment);

			auto *renderCmdList =
				dynamic_cast<IGraphicsCommandList *>(cmdList); // NOLINT(*-pro-type-static-cast-downcast)

			renderCmdList->BeginRenderPass(renderPassDesc);

			// TODO: Bind deferred lighting pipeline and dispatch a full-screen triangle/quad.
			// The pipeline would sample the G-buffer textures (albedo, normal, depth) and
			// accumulate lighting contributions from the scene's light sources.
			//
			// Pseudocode:
			//   renderCmdList->BindPipeline(deferredLightingPipeline);
			//   renderCmdList->SetBindGroup(0, gBufferBindGroup);  // albedo, normal, depth samplers
			//   renderCmdList->SetBindGroup(1, lightDataBindGroup); // light UBO/SSBO
			//   renderCmdList->Draw(3, 1, 0, 0); // full-screen triangle

			renderCmdList->EndRenderPass();
		});

	return lightingPass;
}

static const Hush::Graphics::DefaultRenderer::FinalPassResources &AddFinalPass(
	Hush::WindowRenderer &windowRenderer, Hush::RenderGraph::RenderGraph &graph,
	const Hush::Graphics::DefaultRenderer::LightingPassResources &lightingResources)
{
	using namespace Hush::Graphics;
	using namespace Hush::RenderGraph;
	using namespace Hush::Graphics::DefaultRenderer;

	const auto &finalPass = graph.AddPass<FinalPassResources>(
		EPassType::Graphics, "FinalPass",
		[&windowRenderer, &lightingResources](RenderGraph::BuildContext &ctx, FinalPassResources &data) {
			// Read the lighting result
			ctx.Read(lightingResources.lightingBuffer, EResourceState::PixelShaderAccess);

			// Import the swapchain backbuffer as an external resource.
			// The actual texture pointer will be updated per-frame before execution
			// via RenderGraph::UpdateImport() (driven by the RenderGraphSystem or
			// equivalent frame-update callback).
			IGraphicsTexture *backbufferTexture = windowRenderer.GetGraphicsDevice()->GetCurrentFrameTexture();
			data.backbuffer = ctx.Import<ImportedTextureResource>(
				"Backbuffer",
				ImportedTextureResource{.texture = backbufferTexture},
				EResourceState::RenderTarget);

			// This pass must never be culled — it is the final presentation pass.
			ctx.SetCullingMode(RenderPassNode::EPassCullingMode::NeverCull);
		},
		[&lightingResources](FinalPassResources &data, Hush::Graphics::ICommandList *cmdList,
							 const Hush::RenderGraph::ResourceManager &resourceManager) {
			// Resolve the lighting result for reading
			[[maybe_unused]] auto *lightingTexture =
				resourceManager.GetResource<TextureResource>(lightingResources.lightingBuffer)->texture.get();

			// Resolve the backbuffer for writing
			auto *backbufferTexture =
				resourceManager.GetResource<ImportedTextureResource>(data.backbuffer)->texture;

			// Set up a render pass targeting the swapchain backbuffer to composite the
			// final image. In a complete implementation this would be a full-screen blit
			// (tone-mapping / post-processing) from the HDR lighting buffer to the LDR
			// backbuffer.
			RenderPassDescriptor renderPassDesc{};
			renderPassDesc.debugLabel = "FinalPass";

			RenderPassColorAttachment colorAttachment{};
			colorAttachment.texture = backbufferTexture;
			colorAttachment.loadOp = ELoadOp::Clear;
			colorAttachment.storeOp = EStoreOp::Store;
			colorAttachment.clearValue = {0.0f, 0.0f, 0.0f, 1.0f};
			renderPassDesc.AddColorAttachment(colorAttachment);

			auto *renderCmdList =
				dynamic_cast<IGraphicsCommandList *>(cmdList); // NOLINT(*-pro-type-static-cast-downcast)

			renderCmdList->BeginRenderPass(renderPassDesc);

			// TODO: Bind a full-screen blit / tone-mapping pipeline that samples
			// the HDR lighting buffer and writes the final LDR result to the
			// swapchain backbuffer.
			//
			// Pseudocode:
			//   renderCmdList->BindPipeline(blitPipeline);
			//   renderCmdList->SetBindGroup(0, lightingBufferBindGroup);
			//   renderCmdList->Draw(3, 1, 0, 0); // full-screen triangle

			renderCmdList->EndRenderPass();
		});

	return finalPass;
}

void Hush::Graphics::DefaultRenderer::AddDefaultRenderer(Hush::WindowRenderer &windowRenderer, Hush::Scene *scene)
{
	auto &graph = windowRenderer.GetRenderGraph();

	// 1. Geometry pass — fill the G-buffer (albedo, normals, depth)
	const GeometryPassResources &geometryPassResources = AddGeometryPass(windowRenderer, graph, scene);

	// 2. Lighting pass — read G-buffer, accumulate lighting into an HDR buffer
	const LightingPassResources &lightingPassResources =
		AddLightingPass(windowRenderer, graph, geometryPassResources, scene);

	// 3. Final pass — tone-map / blit the HDR lighting result to the swapchain backbuffer
	AddFinalPass(windowRenderer, graph, lightingPassResources);
}