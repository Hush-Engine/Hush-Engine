#include "TransformationSystem.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/Serializable.hpp"
#include "Components/Transform.hpp"
#include "Components/WorldTransform.hpp"
#include "Mat4Math.hpp"
#include "Profiling.hpp"
#include "Scene.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"
#include <cstdint>

void Hush::TransformationSystem::Init()
{
	Scene &scene = this->GetScene();
	{
		Entity comp = scene.EntityFromIdUnchecked(scene.RegisterComponent<WorldTransform>());
		Serializable& ser = comp.AddComponent<Serializable>();
		ser.serialize = [](const uint8_t* self, Serialization::JsonSerializer& ser){
			const auto* xform = reinterpret_cast<const WorldTransform*>(self);
			const Transform* basePtr = xform;
			Serialization::ESerializationError localErr = ser.Serialize(*basePtr);
			if (localErr != Serialization::ESerializationError::None) {
				return Serializable::EError::ParseError;
			}
			return Serializable::EError::None;
		};
	}
	{
		Entity comp = scene.EntityFromIdUnchecked(scene.RegisterComponent<LocalTransform>());
		Serializable& ser = comp.AddComponent<Serializable>();
		ser.serialize = [](const uint8_t* self, Serialization::JsonSerializer& ser){
			const auto* xform = reinterpret_cast<const LocalTransform*>(self);
			const Transform* basePtr = xform;
			Serialization::ESerializationError localErr = ser.Serialize(*basePtr);
			if (localErr != Serialization::ESerializationError::None) {
				return Serializable::EError::ParseError;
			}
			return Serializable::EError::None;
		};
	}

	this->m_transformableEntitiesQuery = this->GetScene().CreateQuery<WorldTransform, LocalTransform>();
}

void Hush::TransformationSystem::OnShutdown()
{
}

void Hush::TransformationSystem::OnUpdate(float delta)
{
	ZoneScoped;
	(void)delta;
	this->m_transformableEntitiesQuery.Each([](Entity &entity, WorldTransform &worldXform, LocalTransform &localXform) {
		// Get the parents xform and multiply that by the local xform... that now becomes the global xform
		Entity parent = entity.GetParent();
		glm::mat4 worldMatrix = Mat4Math::IDENTITY;
		if (parent.IsValid())
		{
			WorldTransform *parentWorldXform = parent.GetComponent<WorldTransform>();
			worldMatrix = parentWorldXform->GetTransformationMatrix();
		}
		worldXform.SetTransformationMatrix(worldMatrix * localXform.GetTransformationMatrix());
	});
}

void Hush::TransformationSystem::OnFixedUpdate(float delta)
{
	(void)delta;
}

void Hush::TransformationSystem::OnRender()
{
}

void Hush::TransformationSystem::OnPreRender()
{
}

void Hush::TransformationSystem::OnPostRender()
{
}
