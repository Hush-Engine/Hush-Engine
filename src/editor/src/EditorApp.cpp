//
// Created by Alan5 on 22/09/2024.
//

#include "HushEngine.hpp"
#include "IApplication.hpp"
#include "ISystem.hpp"
#include "Scene.hpp"
#include "Shared/EditorCamera.hpp"
#include "TransformationSystem.hpp"
#include "UI.hpp"
#include "VirtualFilesystem.hpp"
#include "components/EditorInfo.hpp"
#include "ResourceManager.hpp"
#include "filesystem/CFileSystem/CFileSystem.hpp"
#include "systems/EditorCameraSystem.hpp"
#include "systems/RenderingSystem.hpp"

// Render graph integration
#include "Components/RenderGraphBuilderComponent.hpp"
#include "WindowRenderer.hpp"
#include "RHI/GraphicsResources.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include "RHI/IGraphicsTexture.hpp"
#include "RenderGraph/RenderGraph.hpp"
#include "Logger.hpp"

#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_sdl2.h>
#include <imgui/backends/imgui_impl_wgpu.h>

#include <algorithm>
#include <memory>

class EditorApp final : public Hush::IApplication
{
public:
	explicit EditorApp(Hush::HushEngine *engine)
		: m_engine(engine),
		  m_scene(std::make_unique<Hush::Scene>(engine, engine->GetEngineThreadPool()))
	{
	}

	EditorApp(const EditorApp &) = delete;
	EditorApp(EditorApp &&) = delete;
	EditorApp &operator=(const EditorApp &) = delete;
	EditorApp &operator=(EditorApp &&) = delete;

	~EditorApp() override
	{
		ImGui_ImplWGPU_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}

	void Init() override
	{
		this->m_cameraSystem = std::make_unique<Hush::EditorCameraSystem>(*this->m_scene);
		auto windowSize = m_engine->GetWindowRenderer()->GetWindowSize();
		m_sceneBufferSize = windowSize; // initial size until the panel reports its own
		this->m_scene->CreateEntityWithName("EditorCamera")
			.EmplaceComponent<Hush::EditorCamera>(45.0f, static_cast<float>(windowSize.x),
												  static_cast<float>(windowSize.y), 0.1f, 1000.0f);
		this->m_scene->AddEngineSystem(new Hush::RenderingSystem(*this->m_scene));
		this->m_scene->AddEngineSystem(new Hush::TransformationSystem(*this->m_scene));
		this->m_scene->AddEngineSystem(this->m_cameraSystem.get());
		Hush::Entity entt = this->m_scene->CreateEntityWithName("EngineManager");
		entt.AddComponent<Hush::EditorInfo>();

		this->m_resourceManager = m_engine->GetResourceManager();
		// this->m_resourceManager = &entt.AddComponent<Hush::ResourceManager>();
		// Hush::VirtualFilesystem &vfs = entt.AddComponent<Hush::VirtualFilesystem>();
		// vfs.MountFileSystem<Hush::CFileSystem>("res://", HUSH_DEFAULT_PROJECT_DIR);
		// vfs.MountFileSystem<Hush::CFileSystem>("engine_res://", "./");

		// ── ImGui initialization ────────────────────────────────────
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO &io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		// Platform backend (SDL2)
		Hush::WindowRenderer *windowRenderer = m_engine->GetWindowRenderer();
		ImGui_ImplSDL2_InitForOther(windowRenderer->GetSDLWindow());

		// Renderer backend (WebGPU / wgpu-native)
		Hush::Graphics::IGraphicsDevice *gfxDevice = windowRenderer->GetGraphicsDevice();
		ImGui_ImplWGPU_InitInfo wgpuInitInfo{};
		wgpuInitInfo.Device = static_cast<WGPUDevice>(gfxDevice->GetNativeHandle());
		wgpuInitInfo.NumFramesInFlight = 3;
		wgpuInitInfo.RenderTargetFormat = WGPUTextureFormat_BGRA8Unorm;
		wgpuInitInfo.DepthStencilFormat = WGPUTextureFormat_Undefined;
		ImGui_ImplWGPU_Init(&wgpuInitInfo);

		// Register the render graph builder component so the RenderGraphSystem
		// picks it up automatically and wires our passes into the frame.
		Hush::Entity renderGraphBuilderEntity = this->m_scene->CreateEntityWithName("EditorRenderGraphBuilder");
		auto &builder = renderGraphBuilderEntity.AddComponent<Hush::RenderGraph::RenderGraphBuilderComponent>();

		builder.builderFunc = [this](Hush::RenderGraph::RenderGraph &graph) { this->SetupRenderGraph(graph); };

		builder.frameUpdateFunc = [this](Hush::RenderGraph::RenderGraph &graph) {
			this->UpdatePerFrameResources(graph);
		};

		this->m_scene->Init();
		this->m_userInterface.Init(this->m_scene.get());
	}

	void Update(float delta) override
	{
		this->m_scene->Update(delta);
	}

	void FixedUpdate(float delta) override
	{
		this->m_scene->FixedUpdate(delta);
	}

	void OnRender(float delta) override
	{
		// Update the scene panel texture BEFORE building ImGui draw data.
		// The texture was realized during the previous frame's OnPreRender
		// (graph compile + resource realization).
		UpdateScenePanelTexture();

		// Start a new ImGui frame (platform + renderer backends + core)
		ImGui_ImplWGPU_NewFrame();
		ImGui_ImplSDL2_NewFrame();
		ImGui::NewFrame();

		// Build the ImGui draw lists.  This must happen BEFORE
		// Scene::Render() so that the draw data is consumed in the same
		// frame it is built — no stale texture references across frames.
		this->m_userInterface.DrawPanels(delta);

		// ── Check if the scene panel was resized ────────────────────
		// DrawPanels just ran ScenePanel::OnRender which may have updated
		// the panel's content size.  If it changed, record the new size
		// and mark the scene buffer dirty so OnPreRender invalidates the
		// render graph BEFORE the next rebuild.  The current frame still
		// renders with the old (valid) texture — no blank frame.
		auto &scenePanel = m_userInterface.GetPanel<Hush::ScenePanel>();
		if (scenePanel.ConsumeResized())
		{
		    const auto newSize = scenePanel.GetPanelSize();
			if (newSize.x > 0 && newSize.y > 0)
			{
    			m_sceneBufferSize = scenePanel.GetPanelSize();
    			m_sceneBufferDirty = true;
			}
		}

		// Execute the render graph.  The ImGui pass inside will call
		// ImGui_ImplWGPU_RenderDrawData with the draw data we just built.
		this->m_scene->Render();
	}

	void OnPostRender() override
	{
		this->m_scene->PostRender();
	}

	void OnPreRender() override
	{
		// If the scene panel was resized last frame, invalidate the render
		// graph BEFORE Scene::PreRender() so the RenderGraphSystem takes
		// the slow path (Reset + rebuild + compile).  The new texture is
		// realized at the updated m_sceneBufferSize.
		if (m_sceneBufferDirty)
		{
			m_sceneBufferDirty = false;

			// Update the editor camera's viewport so the projection
			// matrix uses the correct aspect ratio.
			glm::u32vec2 newSize = m_sceneBufferSize;
			this->m_scene->CreateQuery<Hush::EditorCamera>().Each([&newSize](Hush::Entity &, Hush::EditorCamera &cam) {
				cam.SetViewportSize(static_cast<float>(newSize.x), static_cast<float>(newSize.y));
			});

			m_engine->GetWindowRenderer()->GetRenderDevice().Invalidate();
		}

		// If the render graph is about to be rebuilt (Invalidated by us
		// above, or externally by a window resize), steal the current
		// scene texture out of the graph BEFORE Reset() destroys it.
		// This keeps the GPU texture alive so the ScenePanel's raw
		// WGPUTextureView pointer remains valid during the rebuild frame.
		auto &renderDevice = m_engine->GetWindowRenderer()->GetRenderDevice();
		if (renderDevice.IsDirty())
		{
			CacheCurrentSceneTexture();
		}

		this->m_scene->PreRender();
	}

	void DisposeFrame() override
	{
		this->m_resourceManager->FreePending();
	}

	[[nodiscard]]
	std::string_view GetAppName() const noexcept override
	{
		return "Hush-Editor";
	}

	Hush::Scene *GetScene() noexcept override
	{
		return this->m_scene.get();
	}

private:
	// TODO: the scene renderer might need to be part of the core engine project
	// // so we could use something like a reusable DeferredRenderer or Forward+, etc.
	// instead of creating render passes from scratch in each app.
	void SetupRenderGraph(Hush::RenderGraph::RenderGraph &graph)
	{
		using namespace Hush::RenderGraph;
		using namespace Hush::Graphics;

		IGraphicsDevice *device = m_engine->GetWindowRenderer()->GetGraphicsDevice();

		// ── Pass 1: Scene render ────────────────────────────────────

		struct ScenePassData
		{
			ResourceId renderTexture;
		};

		const auto &scenePassData = graph.AddPass<ScenePassData>(
			EPassType::Graphics, "EditorScenePass",

			// BUILD
			[this](RenderGraph::BuildContext &ctx, ScenePassData &data) {
				// Use the scene panel's content size so the render texture
				// matches the panel 1:1.  Falls back to the window size on
				// the very first frame (before the panel has reported).
				const auto bufferSize = m_sceneBufferSize;

				// Ensure we wait for any resource uploads (textures, buffers)
				// that were queued before this frame.
				ctx.Read(ctx.GetResourceIdByName(RenderGraph::RenderGraph::RESOURCE_UPLOAD_SYNC_TOKEN_NAME));

				data.renderTexture = ctx.Create<TextureResource>(
					"EditorScenePass_RenderTexture", TextureDescriptor{
														 .width = bufferSize.x,
														 .height = bufferSize.y,
														 .format = ETextureFormat::BGRA8_UNORM,
														 .usage = ETextureUsage::RenderTarget | ETextureUsage::Sampled,
													 });

				// Never cull this pass — the editor always needs the scene
				// texture even if nothing else reads it explicitly.
				ctx.SetCullingMode(RenderPassNode::EPassCullingMode::NeverCull);
			},

			// EXECUTE
			[](ScenePassData &data, Hush::Graphics::ICommandList *cmdList,
			   const Hush::RenderGraph::ResourceManager &resourceManager) {
				auto *cmd = dynamic_cast<Hush::Graphics::IGraphicsCommandList *>(cmdList);
				if (cmd == nullptr)
				{
					Hush::LogFormat(Hush::ELogLevel::Error, "[EditorScenePass] Failed to get graphics command list.");
					return;
				}

				RenderPassDescriptor renderPass{};
				renderPass.debugLabel = "EditorScenePass";

				RenderPassColorAttachment colorAttachment{};
				colorAttachment.texture =
					resourceManager.GetResource<TextureResource>(data.renderTexture)->texture.get();
				colorAttachment.loadOp = ELoadOp::Clear;
				colorAttachment.storeOp = EStoreOp::Store;
				colorAttachment.clearValue = ClearColorValue{0.6f, 0.6f, 0.0f, 1.0f};
				renderPass.AddColorAttachment(colorAttachment);

				cmd->BeginRenderPass(renderPass);
				// TODO: actual scene rendering commands go here (deferred /
				//       forward passes, mesh draws, etc.)
				cmd->EndRenderPass();
			});

		// Save the scene texture resource ID so we can look it up later
		// when forwarding the native view to the ScenePanel.
		m_sceneTextureResourceId = scenePassData.renderTexture;

		// ── Pass 2: ImGui render to backbuffer ──────────────────────

		struct ImGuiPassData
		{
			ResourceId sceneTexture; // read-dep on the scene render texture
			ResourceId backbuffer;	 // write to swapchain backbuffer
		};

		const auto &imguiPassData = graph.AddPass<ImGuiPassData>(
			EPassType::Graphics, "EditorImGuiPass",

			// BUILD
			[&scenePassData, device, this](RenderGraph::BuildContext &ctx, ImGuiPassData &data) {
				// Read the scene texture — this creates a dependency so the
				// ImGui pass is guaranteed to run after ScenePass.
				data.sceneTexture = ctx.Read(scenePassData.renderTexture);

				// Import the swapchain backbuffer as an external resource.
				data.backbuffer = ctx.Import<ImportedTextureResource>("EditorBackbuffer",
																	  ImportedTextureResource{
																		  .texture = device->GetCurrentFrameTexture(),
																	  });

				m_backbufferResourceId = data.backbuffer;

				// Never cull the ImGui pass — the editor UI must always be
				// presented.
				ctx.SetCullingMode(RenderPassNode::EPassCullingMode::NeverCull);
			},

			// EXECUTE
			[](ImGuiPassData &data, Hush::Graphics::ICommandList *cmdList,
			   const Hush::RenderGraph::ResourceManager &resourceManager) {
				auto *cmd = dynamic_cast<Hush::Graphics::IGraphicsCommandList *>(cmdList);
				if (cmd == nullptr)
				{
					Hush::LogFormat(Hush::ELogLevel::Error, "[EditorImGuiPass] Failed to get graphics command list.");
					return;
				}

				auto *backbufferTexture =
					resourceManager.GetResource<ImportedTextureResource>(data.backbuffer)->texture;
				if (backbufferTexture == nullptr)
				{
					Hush::LogFormat(Hush::ELogLevel::Error, "[EditorImGuiPass] Backbuffer texture is null.");
					return;
				}

				RenderPassDescriptor renderPass{};
				renderPass.debugLabel = "EditorImGuiPass";

				RenderPassColorAttachment colorAttachment{};
				colorAttachment.texture = backbufferTexture;
				colorAttachment.loadOp = ELoadOp::Clear;
				colorAttachment.storeOp = EStoreOp::Store;
				colorAttachment.clearValue = ClearColorValue{0.06f, 0.06f, 0.08f, 1.0f};
				renderPass.AddColorAttachment(colorAttachment);

				cmd->BeginRenderPass(renderPass);

				// Render ImGui draw data using the WebGPU backend.
				// The draw lists were already built by UI::DrawPanels()
				// earlier in the frame.
				ImDrawData *drawData = ImGui::GetDrawData();
				if (drawData != nullptr)
				{
					// Clamp DisplaySize to the actual backbuffer dimensions.
					// During a window upsize the SDL2 backend may report the
					// new (larger) window size while the surface texture was
					// acquired at the old (smaller) size.  The ImGui WGPU
					// backend computes scissor rects as:
					//   fb = DisplaySize * FramebufferScale
					// If fb exceeds the render target, wgpu rejects the
					// scissor rect.  Adjust DisplaySize so the effective
					// framebuffer size never exceeds the render target.
					auto rtW = static_cast<float>(backbufferTexture->GetWidth());
					auto rtH = static_cast<float>(backbufferTexture->GetHeight());
					float scaleX = (drawData->FramebufferScale.x > 0.0f) ? drawData->FramebufferScale.x : 1.0f;
					float scaleY = (drawData->FramebufferScale.y > 0.0f) ? drawData->FramebufferScale.y : 1.0f;
					float maxDisplayW = rtW / scaleX;
					float maxDisplayH = rtH / scaleY;
					drawData->DisplaySize.x = std::min(drawData->DisplaySize.x, maxDisplayW);
					drawData->DisplaySize.y = std::min(drawData->DisplaySize.y, maxDisplayH);

					// Obtain the underlying WGPURenderPassEncoder from
					// the engine's command list so the ImGui backend can
					// record its draw commands into the active pass.
					auto *nativePass = static_cast<WGPURenderPassEncoder>(cmd->GetNativeRenderPass());
					if (nativePass != nullptr)
					{
						ImGui_ImplWGPU_RenderDrawData(drawData, nativePass);
					}
				}

				cmd->EndRenderPass();
			});

		(void)imguiPassData; // suppress unused warning
	}

	/// @brief Lightweight per-frame update — swap the backbuffer import
	///        without recompiling the graph.
	void UpdatePerFrameResources(Hush::RenderGraph::RenderGraph &graph)
	{
		using namespace Hush::Graphics;

		IGraphicsDevice *device = m_engine->GetWindowRenderer()->GetGraphicsDevice();

		graph.UpdateImport<ImportedTextureResource>(m_backbufferResourceId,
													ImportedTextureResource{
														.texture = device->GetCurrentFrameTexture(),
													});
	}

	/// @brief Steal ownership of the current scene texture from the render
	///        graph so it survives a graph Reset().
	///
	/// Call this *before* Reset() destroys the old resources.  The texture
	/// is kept alive in m_cachedSceneTexture until the new graph realises a
	/// replacement, at which point UpdateScenePanelTexture() releases it.
	void CacheCurrentSceneTexture()
	{
		Hush::WindowRenderer *windowRenderer = m_engine->GetWindowRenderer();
		if (windowRenderer == nullptr)
		{
			return;
		}

		auto &resourceManager = windowRenderer->GetRenderGraph().GetResourceManager();

		auto *texRes = resourceManager.GetResource<Hush::Graphics::TextureResource>(m_sceneTextureResourceId);

		if (texRes != nullptr && texRes->texture != nullptr)
		{
			m_cachedSceneTexture = std::move(texRes->texture);
		}
	}

	void UpdateScenePanelTexture()
	{
		Hush::WindowRenderer *windowRenderer = m_engine->GetWindowRenderer();
		if (windowRenderer == nullptr)
		{
			return;
		}

		auto &resourceManager = windowRenderer->GetRenderGraph().GetResourceManager();

		auto *texRes = resourceManager.GetResource<Hush::Graphics::TextureResource>(m_sceneTextureResourceId);

		if (texRes != nullptr && texRes->texture != nullptr)
		{
			// New texture is realized — swap to it and release the old cache.
			m_cachedSceneTexture.reset();

			Hush::Graphics::IGraphicsTexture *tex = texRes->texture.get();
			void *nativeView = tex->GetNativeView();

			m_userInterface.SetSceneTextureView(nativeView, tex->GetWidth(), tex->GetHeight());
			return;
		}

		// New texture is not realized yet (graph was just rebuilt).
		// If we have a cached copy of the previous texture, keep
		// displaying it — no flicker, no stale pointer.
		if (m_cachedSceneTexture != nullptr)
		{
			void *nativeView = m_cachedSceneTexture->GetNativeView();
			m_userInterface.SetSceneTextureView(nativeView, m_cachedSceneTexture->GetWidth(),
												m_cachedSceneTexture->GetHeight());
			return;
		}

		// No texture available at all (first frame).
		m_userInterface.SetSceneTextureView(nullptr, 0, 0);
	}

	std::unique_ptr<Hush::Scene> m_scene;
	std::unique_ptr<Hush::EditorCameraSystem> m_cameraSystem;

	/// Holds the previous scene texture alive across a render graph rebuild
	/// so the ScenePanel's raw WGPUTextureView pointer doesn't dangle.
	/// Released once the new texture is realized.
	std::unique_ptr<Hush::Graphics::IGraphicsTexture> m_cachedSceneTexture;

	Hush::HushEngine *m_engine = nullptr;

	Hush::UI m_userInterface;
	Hush::ResourceManager *m_resourceManager = nullptr;

	/// Current desired size for the scene render texture (matches the
	/// Scene panel's content region).  Updated each frame after DrawPanels.
	glm::u32vec2 m_sceneBufferSize{0, 0};

	/// Set to true when the scene panel resizes; consumed in OnPreRender
	/// to invalidate the render graph before the next rebuild.
	bool m_sceneBufferDirty = false;

	/// Resource ID of the scene render texture (created in ScenePass).
	Hush::RenderGraph::ResourceId m_sceneTextureResourceId{};

	/// Resource ID of the imported swapchain backbuffer (updated each frame).
	Hush::RenderGraph::ResourceId m_backbufferResourceId{};
};

extern "C" bool BundledAppExists_Internal_() // NOLINT(*-identifier-naming)
{
	return true;
}

extern "C" Hush::IApplication *BundledApp_Internal_(Hush::HushEngine *engine) // NOLINT(*-identifier-naming)
{
	return new EditorApp(engine);
}
