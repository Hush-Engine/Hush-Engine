#include "HushEngine.hpp"
#include "IApplication.hpp"
#include "ISystem.hpp"
#include "Scene.hpp"
#include "RenderGraph/RenderG.hpp"
#include <memory>

class ExampleApp final : public Hush::IApplication
{
public:
	ExampleApp(Hush::HushEngine *engine)
		: m_scene(std::make_unique<Hush::Scene>(engine, engine->GetEngineThreadPool()))
	{
	}

	ExampleApp(const ExampleApp &) = delete;
	ExampleApp(ExampleApp &&) = delete;
	ExampleApp &operator=(const ExampleApp &) = delete;
	ExampleApp &operator=(ExampleApp &&) = delete;

	~ExampleApp() override = default;

	void Init() override
	{
	    using namespace Hush::Exp::RenderGraph;
		Hush::Exp::RenderGraph::RenderGraph graph;

		struct CustomData
		{

		};

		const auto &passData = graph.AddPass<CustomData>(Hush::Exp::RenderGraph::EPassType::Graphics,
		"CustomPass",
            [](RenderGraph::BuildContext& ctx, CustomData& data)
            {
                std::cout << "  [Build] Post-Process Pass\n";

                // data.finalOutput = ctx.Create<Texture>("Final Image", Texture::Descriptor{
                //     .width = 1920,
                //     .height = 1080,
                //     .format = 0, // RGBA8 LDR
                //     .name = "FinalImage"
                // });
            },
            [](CustomData& data, void* ctx)
            {
                // auto* renderCtx = static_cast<MockRenderContext*>(ctx);
                // std::cout << "  [Execute] Post-Process Pass (Frame " << renderCtx->frameNumber << ")\n";
                // std::cout << "    - Applying bloom\n";
                // std::cout << "    - Tone mapping HDR -> LDR\n";
                // std::cout << "    - Final output ready for presentation!\n";
            });
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
		this->m_scene->Render();
	}

	void OnPostRender() override
	{
		this->m_scene->PostRender();
	}

	void OnPreRender() override
	{
		this->m_scene->PreRender();
	}

	void DisposeFrame() override
	{
	}

	[[nodiscard]]
	std::string_view GetAppName() const noexcept override
	{
		return "Hush Example";
	}

	Hush::Scene *GetScene() noexcept override
	{
		return this->m_scene.get();
	}

private:
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
