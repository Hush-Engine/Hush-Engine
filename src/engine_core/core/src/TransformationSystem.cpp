#include "TransformationSystem.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "Mat4Math.hpp"
#include "Scene.hpp"

void Hush::TransformationSystem::Init()
{
	this->m_transformableEntitiesQuery = this->GetScene().CreateQuery<WorldTransform, LocalTransform>();
}

void Hush::TransformationSystem::OnShutdown()
{
}

void Hush::TransformationSystem::OnUpdate(float delta)
{
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
