#include "Scene.hpp"
#include "EntityManagerSystem.hpp"

void Hush::EntityManagerSystem::Init() {
	this->m_markedForDeletion = this->GetScene().CreateQuery<EntityMarkedForDeletion>();
}

void Hush::EntityManagerSystem::OnShutdown() {
	
}


void Hush::EntityManagerSystem::OnUpdate(float delta) {
	(void)delta;
	this->m_markedForDeletion.Each([this](Entity& ent, EntityMarkedForDeletion&){
	    this->GetScene().DestroyEntity(ent);
	});
}


void Hush::EntityManagerSystem::OnFixedUpdate(float delta) {
	(void)delta;
}


void Hush::EntityManagerSystem::OnRender() {
	
}

/// OnPreRender() is called before rendering.
void Hush::EntityManagerSystem::OnPreRender() {
	
}

/// OnPostRender() is called after rendering.
void Hush::EntityManagerSystem::OnPostRender() {
	
}

