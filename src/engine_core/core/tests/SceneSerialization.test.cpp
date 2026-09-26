#include "ISystem.hpp"
#include "Scene.hpp"

#include <catch2/catch_test_macros.hpp>

namespace
{
	class SceneAssetTestSystem final : public Hush::ISystem
	{
	public:
		explicit SceneAssetTestSystem(Hush::Scene &scene, int *initCalls = nullptr, int *shutdownCalls = nullptr,
									  bool *sawLoadedEntity = nullptr)
			: ISystem(scene),
			  m_initCalls(initCalls),
			  m_shutdownCalls(shutdownCalls),
			  m_sawLoadedEntity(sawLoadedEntity)
		{
		}

		void Init() override
		{
			if (m_initCalls != nullptr)
			{
				++*m_initCalls;
			}
			if (m_sawLoadedEntity != nullptr)
			{
				*m_sawLoadedEntity = GetScene().Lookup("Loaded") != Hush::Entity::INVALID_ENTITY_ID;
			}
		}
		void OnShutdown() override
		{
			if (m_shutdownCalls != nullptr)
			{
				++*m_shutdownCalls;
			}
		}
		void OnUpdate(float) override
		{
		}
		void OnFixedUpdate(float) override
		{
		}
		void OnRender() override
		{
		}
		void OnPreRender() override
		{
		}
		void OnPostRender() override
		{
		}

		std::string_view GetName() const override
		{
			return "Game.SceneAssetTestSystem";
		}

	private:
		int *m_initCalls;
		int *m_shutdownCalls;
		bool *m_sawLoadedEntity;
	};

	struct FailingComponent
	{
		int value = 0;
	};
} // namespace

TEST_CASE("Scene assets persist stable module system references", "[scene][serialization]")
{
	Hush::Scene scene(nullptr, nullptr);
	scene.AddSystem(std::make_unique<SceneAssetTestSystem>(scene),
					Hush::SerializedSystem{.module = "Game", .type = "Game.SceneAssetTestSystem"});

	std::string asset;
	REQUIRE(scene.ToSceneAsset(asset) == Hush::Scene::EError::None);

	Hush::Scene restored(nullptr, nullptr);
	std::string restoredModule;
	std::string restoredType;
	restored.SetSystemFactory(
		[&](Hush::Scene &target, std::string_view module, std::string_view type) -> std::unique_ptr<Hush::ISystem> {
			restoredModule = module;
			restoredType = type;
			return std::make_unique<SceneAssetTestSystem>(target);
		});
	REQUIRE(restored.FromSceneAsset(asset) == Hush::Scene::EError::None);
	REQUIRE(restoredModule == "Game");
	REQUIRE(restoredType == "Game.SceneAssetTestSystem");
}

TEST_CASE("Scene assets resolve systems independent of root field order", "[scene][serialization]")
{
	Hush::Scene scene(nullptr, nullptr);
	std::string resolvedModule;
	std::string resolvedType;
	scene.SetSystemFactory(
		[&](Hush::Scene &target, std::string_view module, std::string_view type) -> std::unique_ptr<Hush::ISystem> {
			resolvedModule = module;
			resolvedType = type;
			return std::make_unique<SceneAssetTestSystem>(target);
		});

	const std::string asset = R"({"entities":[],"systems":[{"type":"Game.SceneAssetTestSystem","module":"Game"}]})";
	REQUIRE(scene.FromSceneAsset(asset) == Hush::Scene::EError::None);
	REQUIRE(resolvedModule == "Game");
	REQUIRE(resolvedType == "Game.SceneAssetTestSystem");

	std::string roundTrip;
	REQUIRE(scene.ToSceneAsset(roundTrip) == Hush::Scene::EError::None);
	REQUIRE(roundTrip.find("Game.SceneAssetTestSystem") != std::string::npos);
}

TEST_CASE("Scene assets remain compatible with entity-only files", "[scene][serialization]")
{
	Hush::Scene scene(nullptr, nullptr);
	REQUIRE(scene.FromSceneAsset(R"({"entities":[]})") == Hush::Scene::EError::None);
	REQUIRE(scene.FromSceneAsset(R"({"systems":[{"module":"Missing","type":"Missing.System"}],"entities":[]})") ==
			Hush::Scene::EError::SystemResolutionFailed);
}

TEST_CASE("Scene assets validate entities before resolving systems", "[scene][serialization]")
{
	Hush::Scene scene(nullptr, nullptr);
	int factoryCalls = 0;
	scene.SetSystemFactory(
		[&](Hush::Scene &target, std::string_view, std::string_view) -> std::unique_ptr<Hush::ISystem> {
			++factoryCalls;
			return std::make_unique<SceneAssetTestSystem>(target);
		});

	REQUIRE(scene.FromSceneAsset(R"({"systems":[{"module":"Game","type":"Game.System"}]})") ==
			Hush::Scene::EError::BadSceneFormat);
	REQUIRE(factoryCalls == 0);
}

TEST_CASE("Scene assets initialize systems only after components deserialize", "[scene][serialization]")
{
	using Hush::Threading::Executors::ThreadPool;
	ThreadPool threadPool = ThreadPool::Create({.numThreads = 1, .pinToCore = false});
	Hush::Scene scene(nullptr, &threadPool);

	const Hush::Entity::EntityId componentId = scene.RegisterComponent<FailingComponent>();
	Hush::Entity componentType = scene.EntityFromIdUnchecked(componentId);
	Hush::Serializable &serializer = componentType.AddComponent<Hush::Serializable>();
	serializer.deserialize = [](std::uint8_t *, Hush::Serialization::JsonDeserializer &, void *) {
		return Hush::Serializable::EError::ParseError;
	};
	serializer.type = componentId;
	const std::string componentKey(componentType.GetKey());

	int initCalls = 0;
	int shutdownCalls = 0;
	scene.SetSystemFactory(
		[&](Hush::Scene &target, std::string_view, std::string_view) -> std::unique_ptr<Hush::ISystem> {
			return std::make_unique<SceneAssetTestSystem>(target, &initCalls, &shutdownCalls);
		});
	scene.Init();

	const std::string asset =
		R"({"systems":[{"module":"Game","type":"Game.System"}],"entities":[{"key":"Entity","components":[{"key":")" +
		componentKey + R"("}]}]})";
	REQUIRE(scene.FromSceneAsset(asset) == Hush::Scene::EError::BadSceneFormat);
	REQUIRE(initCalls == 0);
	REQUIRE(shutdownCalls == 0);

	std::string roundTrip;
	REQUIRE(scene.ToSceneAsset(roundTrip) == Hush::Scene::EError::None);
	REQUIRE(roundTrip.find("Game.System") == std::string::npos);
}

TEST_CASE("Scene asset systems initialize after loaded entities exist", "[scene][serialization]")
{
	int initCalls = 0;
	bool sawLoadedEntity = false;
	using Hush::Threading::Executors::ThreadPool;
	ThreadPool threadPool = ThreadPool::Create({.numThreads = 1, .pinToCore = false});
	Hush::Scene scene(nullptr, &threadPool);
	scene.SetSystemFactory(
		[&](Hush::Scene &target, std::string_view, std::string_view) -> std::unique_ptr<Hush::ISystem> {
			return std::make_unique<SceneAssetTestSystem>(target, &initCalls, nullptr, &sawLoadedEntity);
		});
	scene.Init();

	const std::string asset =
		R"({"systems":[{"module":"Game","type":"Game.System"}],"entities":[{"key":"Loaded","components":[]}]})";
	REQUIRE(scene.FromSceneAsset(asset) == Hush::Scene::EError::None);
	REQUIRE(initCalls == 1);
	REQUIRE(sawLoadedEntity);
}
