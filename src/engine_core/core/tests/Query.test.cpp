/*! \file Query.test.cpp
	\author Alan Ramirez
	\date 2025-02-10
	\brief Qery test implementation
*/
#include <Query.hpp>
#include <Scene.hpp>

#include <catch2/catch_test_macros.hpp>

struct Position
{
	float x;
	float y;
};
struct Velocity
{
	float x;
	float y;
};

TEST_CASE("Query creation", "[query]")
{
	SECTION("CreateQuery")
	{
		using Hush::Threading::Executors::ThreadPool;

		ThreadPool threadPool = ThreadPool::Create({.numThreads = 1, .pinToCore = false});
		Hush::Scene scene(nullptr, &threadPool);

		Hush::Query<Position, Velocity> query = scene.CreateQuery<Position, Velocity>();

		REQUIRE(query.begin().Size() == 0);
	}

	SECTION("Create query with entities")
	{
		using Hush::Threading::Executors::ThreadPool;

		ThreadPool threadPool = ThreadPool::Create({.numThreads = 1, .pinToCore = false});

		Hush::Scene scene(nullptr, &threadPool);

		Hush::Entity entity = scene.CreateEntity();
		entity.EmplaceComponent<Position>(1.0f, 2.0f);
		entity.EmplaceComponent<Velocity>(3.0f, 4.0f);

		Hush::Query<Position, Velocity> query = scene.CreateQuery<Position, Velocity>();

		Position storedPosition{};
		Velocity storedVelocity{};

		for (auto [positions, velocities] : query)
		{
			for (std::size_t i = 0; i < positions.size(); ++i)
			{
				storedPosition = positions[i];
				storedVelocity = velocities[i];
			}
		}

		REQUIRE(storedPosition.x == 1.0f);
		REQUIRE(storedPosition.y == 2.0f);
		REQUIRE(storedVelocity.x == 3.0f);
		REQUIRE(storedVelocity.y == 4.0f);
	}

	SECTION("Iterate using Each")
	{
		using Hush::Threading::Executors::ThreadPool;

		ThreadPool threadPool = ThreadPool::Create({.numThreads = 1, .pinToCore = false});

		Hush::Scene scene(nullptr, &threadPool);
		constexpr std::size_t NUM_ENTITIES = 1000;

		for (std::size_t i = 0; i < NUM_ENTITIES; ++i)
		{
			Hush::Entity entity = scene.CreateEntity();
			entity.EmplaceComponent<Position>(static_cast<float>(i), static_cast<float>(i));
			entity.EmplaceComponent<Velocity>(static_cast<float>(i), static_cast<float>(i));
		}

		Hush::Query<Position, Velocity> query = scene.CreateQuery<Position, Velocity>();

		std::size_t numEntities = 0;
		query.Each([&numEntities](const Position &position, const Velocity &velocity) {
			(void)position;
			(void)velocity;
			++numEntities;
		});

		REQUIRE(numEntities == NUM_ENTITIES);
	}

	SECTION("Different components")
	{
		using Hush::Threading::Executors::ThreadPool;

		ThreadPool threadPool = ThreadPool::Create({.numThreads = 1, .pinToCore = false});

		Hush::Scene scene(nullptr, &threadPool);

		constexpr std::size_t NUM_ENTITIES_WITH_POSITION = 1000;
		constexpr std::size_t NUM_ENTITIES_WITH_VELOCITY = 500;
		constexpr std::size_t NUM_ENTITIES_WITH_BOTH = 250;

		for (std::size_t i = 0; i < NUM_ENTITIES_WITH_POSITION; ++i)
		{
			Hush::Entity entity = scene.CreateEntity();
			entity.EmplaceComponent<Position>(static_cast<float>(i), static_cast<float>(i));
		}

		for (std::size_t i = 0; i < NUM_ENTITIES_WITH_VELOCITY; ++i)
		{
			Hush::Entity entity = scene.CreateEntity();
			entity.EmplaceComponent<Velocity>(static_cast<float>(i), static_cast<float>(i));
		}

		for (std::size_t i = 0; i < NUM_ENTITIES_WITH_BOTH; ++i)
		{
			Hush::Entity entity = scene.CreateEntity();
			entity.EmplaceComponent<Position>(static_cast<float>(i), static_cast<float>(i));
			entity.EmplaceComponent<Velocity>(static_cast<float>(i), static_cast<float>(i));
		}

		Hush::Query<Position> queryPosition = scene.CreateQuery<Position>();
		Hush::Query<Velocity> queryVelocity = scene.CreateQuery<Velocity>();
		Hush::Query<Position, Velocity> queryBoth = scene.CreateQuery<Position, Velocity>();

		std::size_t numEntitiesPosition = 0;
		queryPosition.Each([&numEntitiesPosition](const Position &position) {
			(void)position;
			++numEntitiesPosition;
		});

		std::size_t numEntitiesVelocity = 0;
		queryVelocity.Each([&numEntitiesVelocity](const Velocity &velocity) {
			(void)velocity;
			++numEntitiesVelocity;
		});

		std::size_t numEntitiesBoth = 0;
		queryBoth.Each([&](const Position &position, const Velocity &velocity) {
			(void)position;
			(void)velocity;
			++numEntitiesBoth;
		});

		REQUIRE(numEntitiesPosition == NUM_ENTITIES_WITH_POSITION + NUM_ENTITIES_WITH_BOTH);
		REQUIRE(numEntitiesVelocity == NUM_ENTITIES_WITH_VELOCITY + NUM_ENTITIES_WITH_BOTH);
		REQUIRE(numEntitiesBoth == NUM_ENTITIES_WITH_BOTH);
	}
}
