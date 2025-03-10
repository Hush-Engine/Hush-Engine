#include "HierarchyPanel.hpp"
#include <imgui/imgui.h>
#include <Assertions.hpp>
#include "InspectorPanel.hpp"
#include "UI.hpp"

constexpr ImGuiWindowFlags DOCK_BASE_FLAGS =
	ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;


void Hush::HierarchyPanel::Init(Scene* activeScene) noexcept {
	this->m_activeScene = activeScene;
}

void Hush::HierarchyPanel::OnRender()
{
	ImGuiViewport *mainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowViewport(mainViewport->ID);
	ImGui::Begin("Hierarchy");
	
	for (const auto& kv : this->m_activeScene->GetAllEntities()) {
		if (!ImGui::Selectable(kv.first.c_str())) {
			continue;
		}
		UI::Get().GetPanel<InspectorPanel>().SetInspectTarget(kv.second);
	}
	
	ImGui::End();
}
