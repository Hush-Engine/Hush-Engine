#include "HushEngine.hpp"
#include "IApplication.hpp"
#include "ISystem.hpp"
#include "RHI/GraphicsResources.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Systems/RenderGraphSystem.hpp"
#include "Scene.hpp"
#include "WindowRenderer.hpp"
#include <memory>
#include <iostream>

class ExampleApp final : public Hush::IApplication
{
public:
	ExampleApp(Hush::HushEngine *engine)
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
		this->m_scene->AddEngineSystem(new Hush::Graphics::RenderGraphSystem(
			*this->m_scene, &this->m_engine->GetWindowRenderer()->GetRenderGraph()));

		Hush::Entity renderGraphBuilderEntity = this->m_scene->CreateEntityWithName("RenderGraphBuilder");
		auto &builder = renderGraphBuilderEntity.AddComponent<Hush::RenderGraph::RenderGraphBuilderComponent>();

		builder.builderFunc = [this](Hush::RenderGraph::RenderGraph &graph) { this->SetupRenderGraph(graph); };

		this->m_scene->Init();
	}

	void SetupRenderGraph(Hush::RenderGraph::RenderGraph &graph)
	{
		using namespace Hush::RenderGraph;
		using namespace Hush::Graphics;

		// Create render graph
		IGraphicsDevice *device = m_engine->GetWindowRenderer()->GetGraphicsDevice();

		struct ClearPassData
		{
			ResourceId renderTexture;
		};

		const auto &clearPassData = graph.AddPass<ClearPassData>(
			EPassType::Graphics, "ClearPass",
			// BUILD PHASE: Declare resource usage
			[this](RenderGraph::BuildContext &ctx, ClearPassData &data) {
				int32_t width = 0;
				int32_t height = 0;

				this->m_engine->GetWindowRenderer()->GetWindowSize(&width, &height);

				// Create intermediate render texture
				data.renderTexture = ctx.Create<TextureResource>(
					"ClearPass_RenderTexture", TextureDescriptor{
												   .width = static_cast<uint32_t>(width),
												   .height = static_cast<uint32_t>(height),
												   .format = ETextureFormat::BGRA8_UNORM,
												   .usage = ETextureUsage::RenderTarget | ETextureUsage::CopySource,
											   });

				// This pass should never be culled
				// ctx.SetCullingMode(RenderPassNode::EPassCullingMode::NeverCull);
			},
			// EXECUTE PHASE: Perform rendering
			[](ClearPassData &data, Hush::Graphics::ICommandList *cmdList,
			   const Hush::RenderGraph::ResourceManager &resourceManager) {
				auto *cmd = dynamic_cast<Hush::Graphics::IGraphicsCommandList *>(cmdList);

				// Build render pass descriptor
				RenderPassDescriptor renderPass{};
				renderPass.debugLabel = "ClearPass";

				RenderPassColorAttachment colorAttachment{};
				colorAttachment.texture =
					resourceManager.GetResource<TextureResource>(data.renderTexture)->texture.get();
				colorAttachment.loadOp = ELoadOp::Clear;
				colorAttachment.storeOp = EStoreOp::Store;
				colorAttachment.clearValue = ClearColorValue{0.1f, 0.2f, 0.3f, 1.0f}; // Clear to a dark blue color
				renderPass.AddColorAttachment(colorAttachment);

				// Execute render pass
				cmd->BeginRenderPass(renderPass);
				// No draw calls - just clearing
				cmd->EndRenderPass();
			});

		struct CopyToBackbufferPassData
		{
			ResourceId renderTexture;
			ResourceId backbuffer;
		};

		graph.AddPass<CopyToBackbufferPassData>(
			EPassType::Transfer, "CopyToBackbuffer",
			// BUILD PHASE
			[&clearPassData, device](RenderGraph::BuildContext &ctx, CopyToBackbufferPassData &data) {
				// Read from post-process output
				data.renderTexture = ctx.Read(clearPassData.renderTexture);
				data.backbuffer =
					ctx.Import<ImportedTextureResource>("Backbuffer", ImportedTextureResource{
																		  .texture = device->GetCurrentFrameTexture(),
																	  });
			},
			// EXECUTE PHASE
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

	void Update(float delta) override
	{
		GetScene()->Update(delta);
	}

	void FixedUpdate(float delta) override
	{
		GetScene()->FixedUpdate(delta);
	}

	void OnRender(float delta) override
	{
		GetScene()->Render();
	}

	void OnPostRender() override
	{
		GetScene()->PostRender();
	}

	void OnPreRender() override
	{
		GetScene()->PreRender();
	}

	void DisposeFrame() override
	{
	}

	[[nodiscard]]
	std::string_view GetAppName() const noexcept override
	{
		return "Hush RenderGraph + RHI Demo";
	}

	Hush::Scene *GetScene() noexcept override
	{
		return this->m_scene.get();
	}

private:
	Hush::HushEngine *m_engine;
	std::unique_ptr<Hush::Scene> m_scene;
};

extern "C" bool BundledAppExists_Internal_() // NOLINT(*-identifier-naming)
{
	return true;
}

extern "C" Hush::IApplication *BundledApp_Internal_(Hush::HushEngine *engine) // NOLINT(*-identifier-naming)
{
	return new ExampleApp(engine);
}
