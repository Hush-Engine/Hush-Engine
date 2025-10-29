#include "HierarchyPanel.hpp"
#include <cstdint>
#include <imgui/imgui.h>
#include <Assertions.hpp>
#include <optional>
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "InspectorPanel.hpp"
#include "UI.hpp"

void Hush::HierarchyPanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;
	this->m_inspectableEntitiesQuery = this->m_activeScene->CreateQuery<WorldTransform, LocalTransform, Entity::Name>();
}

void Hush::HierarchyPanel::OnRender(float deltaTime)
{
	ImGuiViewport *mainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowViewport(mainViewport->ID);
	ImGui::Begin("Hierarchy");
	auto &inspectorPanel = UI::Get().GetPanel<InspectorPanel>();
	this->m_inspectableEntitiesQuery.Each([&inspectorPanel](Entity &entity, WorldTransform &_, LocalTransform &localxForm, Entity::Name& name) {
		bool selected = inspectorPanel.GetInspectTarget().has_value() &&
						inspectorPanel.GetInspectTarget()->GetId() == entity.GetId();
		// Get the selectable that's related to the parent
		if (localxForm.HasParent())
		{
			// localxForm.GetParentId();
		}
		auto narrowedId = static_cast<int32_t>(entity.GetId());
		ImGui::PushID(narrowedId);
		if (ImGui::Selectable(name.name.data(), selected))
		{
			inspectorPanel.SetInspectTarget(entity.GetId());
		}
		ImGui::PopID();
	});

	ImGui::End();
}
