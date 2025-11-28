#include "HushEngine.hpp"
#include "IApplication.hpp"
#include "ISystem.hpp"
#include "RenderGraph/ResourceId.hpp"
#include "Scene.hpp"
#include "RenderGraph/RenderGraph.hpp"

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
		Hush::RenderGraph::RenderGraph graph;

		struct PassData
		{
			Hush::RenderGraph::ResourceId inputResource;
			Hush::RenderGraph::ResourceId outputResource;
		};
		auto &passData = graph.AddPass<PassData>(
			"My Pass",
			[](Hush::RenderGraph::RenderGraph::BuildContext &ctx, PassData &passData) {
				passData.inputResource = ctx.Create<PassData>("", {});
			},
			[](PassData &data, void *ctx) {

			},
			Hush::RenderGraph::EPassType::Graphics);
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
