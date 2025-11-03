#include "TransformationSystem.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "EcsTerms.hpp"
#include "Scene.hpp"

void Hush::TransformationSystem::Init()
{
	Entity childOfRel = this->GetScene().EntityFromId(EcsTerms::CHILD_OF).value();
	this->m_parentedEntitiesQuery = this->GetScene()
		.CreateQueryBuilder<WorldTransform, LocalTransform>()
		.WithRelationship(childOfRel)
		.Build();
	
}

void Hush::TransformationSystem::OnShutdown()
{
}

void Hush::TransformationSystem::OnUpdate(float delta)
{
	(void)delta;
	this->m_parentedEntitiesQuery.Each([](Entity& entity, WorldTransform& worldXform, LocalTransform& localXform) {
		// Get the parents xform and multiply that by the local xform... that now becomes the global xform
		Entity parent = entity.GetParent();
		if (parent.GetId() == Entity::INVALID_ENTITY) {
			return;
		}
		WorldTransform* parentWorldXform = parent.GetComponent<WorldTransform>();
		worldXform.SetTransformationMatrix(parentWorldXform->XForm(localXform));
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
