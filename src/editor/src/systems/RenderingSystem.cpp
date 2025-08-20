#include "RenderingSystem.hpp"
#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "Logger.hpp"
#include "Query.hpp"
#include "WindowManager.hpp"


void Hush::RenderingSystem::Init() {
	this->GetScene().AddComponentObserver<MeshReference>(EComponentObserverType::Add, [](Entity::EntityId _, MeshReference* mesh) {
	   // Add its values to the renderer
	   LogFormat(ELogLevel::Info, "Hey, we added a mesh");
	});
}

void Hush::RenderingSystem::OnShutdown() {
	
}

void Hush::RenderingSystem::OnUpdate(float delta) {
	
}

void Hush::RenderingSystem::OnFixedUpdate(float delta) {
	
}

void Hush::RenderingSystem::OnRender() {
	
}

void Hush::RenderingSystem::OnPreRender() {
	// TODO: Update camera view matrix and everything else in the scene data here
	IRenderer* renderer = WindowManager::GetMainWindow()->GetInternalRenderer();
	renderer->ClearDrawContext();
	this->m_renderableTargetsQuery.Each([&renderer](Entity& _, const MeshReference& mesh, const WorldTransform& xform) {
	    renderer->PushMesh(&xform, mesh.GetMesh().Get());
    });
}

void Hush::RenderingSystem::OnPostRender() {
	
}

std::string_view Hush::RenderingSystem::GetName() const {
	return "RenderingSystem";
}
