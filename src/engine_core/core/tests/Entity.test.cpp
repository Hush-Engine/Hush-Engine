/*! \file Entity.test.cpp
    \author Alan Ramirez
    \date 2025-02-10
    \brief Entity test implementation
*/
#include <Entity.hpp>
#include <Scene.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Entity creation", "[entity]")
{
    Hush::Scene scene(nullptr);

    Hush::Entity entity = scene.CreateEntity();
    Hush::Entity entity2 = scene.CreateEntityWihName("MyEntity");

    SECTION("CreateEntity")
    {
        REQUIRE(entity.GetId() != 0);
    }

    SECTION("CreateEntityWithName")
    {
        REQUIRE(entity2.GetName().has_value());
        REQUIRE(entity2.GetName().value() == "MyEntity");
    }

    SECTION("DifferentEntities")
    {
        REQUIRE(entity.GetId() != entity2.GetId());
    }

    SECTION("DestroyEntity")
    {
        scene.DestroyEntity(std::move(entity));
        REQUIRE(entity.GetId() == 0);
    }
}

TEST_CASE("Entity with components", "[entity]")
{
    struct Position
    {
        float x;
        float y;
    };

    struct TestA
    {
        float a;

        TestA()
            : a(0.0f)
        {
        }
    };

    SECTION("EmplaceComponent")
    {
        Hush::Scene scene(nullptr);
        Hush::Entity entity = scene.CreateEntity();
        entity.EmplaceComponent<Position>(1.0f, 2.0f);

        scene.CreateEntity().EmplaceComponent<Position>(3.0f, 4.0f);

        auto position = entity.GetComponent<Position>();

        REQUIRE(position != nullptr);
        REQUIRE(position->x == 1.0f);
        REQUIRE(position->y == 2.0f);
    }

    SECTION("AddComponent")
    {
        Hush::Scene scene(nullptr);
        Hush::Entity entity = scene.CreateEntity();
        entity.AddComponent<TestA>();

        auto testA = entity.GetComponent<TestA>();

        REQUIRE(testA != nullptr);
        REQUIRE(testA->a == 0.0f);
    }

    SECTION("RemoveComponent")
    {
        Hush::Scene scene(nullptr);
        Hush::Entity entity = scene.CreateEntity();
        entity.EmplaceComponent<Position>(1.0f, 2.0f);

        REQUIRE(entity.RemoveComponent<Position>());
        REQUIRE(entity.GetComponent<Position>() == nullptr);
    }

    SECTION("RemoveComponentNotAdded")
    {
        Hush::Scene scene(nullptr);
        Hush::Entity entity = scene.CreateEntity();

        REQUIRE_FALSE(entity.RemoveComponent<Position>());
    }

    SECTION("RemoveComponentTwice")
    {
        Hush::Scene scene(nullptr);
        Hush::Entity entity = scene.CreateEntity();
        entity.EmplaceComponent<Position>(1.0f, 2.0f);

        REQUIRE(entity.RemoveComponent<Position>());
        REQUIRE_FALSE(entity.RemoveComponent<Position>());
    }
}