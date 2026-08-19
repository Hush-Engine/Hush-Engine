#include "TransformationSystem.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/Serializable.hpp"
#include "Components/Transform.hpp"
#include "Components/WorldTransform.hpp"
#include "Mat4Math.hpp"
#include "Profiling.hpp"
#include "Scene.hpp"
#include "serialization/Deserialization.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"
#include <cstdint>

// Helper function so things don't get repetitive, this is needed bc World and Local xform derive from the Transform class
template <class T>
inline void RegisterSerializerForXformComp(Hush::Scene &scene)
{
	using namespace Hush;
	Entity comp = scene.EntityFromIdUnchecked(scene.RegisterComponent<T>());
	Serializable &ser = comp.AddComponent<Serializable>();
	ser.serialize = [](const uint8_t *self, Serialization::JsonSerializer &ser, void* ctx) {
		(void)ctx;
		const auto *xform = reinterpret_cast<const T *>(self);
		const Transform *basePtr = xform;
		Serialization::ESerializationError localErr = ser.Serialize(*basePtr, false);
		if (localErr != Serialization::ESerializationError::None)
		{
			return Serializable::EError::ParseError;
		}
		return Serializable::EError::None;
	};
	ser.deserialize = [](uint8_t* self, Serialization::JsonDeserializer &deser, void* ctx) {
		(void)ctx;
		auto *xform = reinterpret_cast<T *>(self);
		Transform *basePtr = xform;
		auto err = deser.Deserialize<Transform>(basePtr);
		if (err != Serialization::EDeserializationError::None) {
			return Serializable::EError::ParseError;
		}
		return Serializable::EError::None;
	};
}

void Hush::TransformationSystem::Init()
{
	Scene &scene = this->GetScene();
	RegisterSerializerForXformComp<WorldTransform>(scene);
	RegisterSerializerForXformComp<LocalTransform>(scene);

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
