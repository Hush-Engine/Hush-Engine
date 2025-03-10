#include "InspectorPanel.hpp"
#include "Components/Transform.hpp"
#include "imgui/imgui.h"
#include <optional>
#include "UI.hpp"

void Hush::InspectorPanel::OnRender() {
	ImGui::Begin("Inspector");
	this->RenderProperties();
	ImGui::End();
}

void Hush::InspectorPanel::Init(Scene* activeScene) noexcept {
	this->m_activeScene = activeScene;
}


void Hush::InspectorPanel::SetInspectTarget(Entity::EntityId entity) {
	this->m_inspectTarget = this->m_activeScene->EntityFromId(entity);
}

const std::optional<Hush::Entity>& Hush::InspectorPanel::GetInspectTarget() {
	return this->m_inspectTarget;
}

void Hush::InspectorPanel::RenderProperties() {	
	if (!this->m_inspectTarget.has_value()) {
		return;
	}

	ImGui::Text("%s", this->m_inspectTarget->GetName().value_or("").data());
	Transform *transform = this->m_inspectTarget->GetComponent<Transform>();
	UI::SerializeComponent(transform, UI::ESerializableComponentType::Transform);
}

