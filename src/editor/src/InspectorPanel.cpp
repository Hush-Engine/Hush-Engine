#include "InspectorPanel.hpp"
#include "imgui/imgui.h"


void Hush::InspectorPanel::OnRender() {
	ImGui::Begin("Inspector");
	ImGui::End();
}

void Hush::InspectorPanel::Init(Scene* activeScene) noexcept {
	
}


void Hush::InspectorPanel::SetInspectTarget(Entity* entity) {
	this->m_inspectTarget = entity;
}


void Hush::InspectorPanel::RenderProperties() {	
	if (this->m_inspectTarget == nullptr) {
		return;
	}
	ImGui::Text("%s", this->m_inspectTarget->GetName().value_or("").data());
}

