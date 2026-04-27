
#include "HierarchyPanel.hpp"
#include <cstdint>
#include <functional>
#include <imgui/imgui.h>
#include <Assertions.hpp>
#include <optional>
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "Entity.hpp"
#include "InspectorPanel.hpp"
#include "UI.hpp"

void Hush::HierarchyPanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;
	this->m_inspectableEntitiesQuery = this->m_activeScene->CreateQuery<WorldTransform, LocalTransform, Entity::Name>();
}

void Hush::HierarchyPanel::OnRender([[maybe_unused]] float deltaTime)
{
	ImGuiViewport *mainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowViewport(mainViewport->ID);
	ImGui::Begin("Hierarchy");
	auto &inspectorPanel = UI::Get().GetPanel<InspectorPanel>();
	// Maybe draw them in separate systems?
	this->m_inspectableEntitiesQuery.Each(
		[&inspectorPanel, this](Entity &entity, [[maybe_unused]] WorldTransform &_, [[maybe_unused]] LocalTransform &localxForm, Entity::Name &name) {
			if (entity.GetParent().IsValid())
			{
				// We skip rendering the entity because the parent would have already rendered itj
				return;
			}
			// bool selected = inspectorPanel.GetInspectTarget().has_value() &&
			// 				inspectorPanel.GetInspectTarget()->GetId() == entity.GetId();
			auto narrowedId = static_cast<int32_t>(entity.GetId());
			ImGui::PushID(narrowedId);

			this->GenerateEntitySelectableTree(entity, name, &inspectorPanel);

			ImGui::PopID();
		});

	ImGui::End();
}

void Hush::HierarchyPanel::GenerateEntitySelectableTree(const Entity &entity, const Entity::Name &name,
														InspectorPanel *inspector)
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
	if (entity.GetChildCount() < 1)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	if (ImGui::TreeNodeEx(name.name.data(), flags))
	{
		if (ImGui::IsItemClicked())
		{
			inspector->SetInspectTarget(entity.GetId());
		}
		entity.EachChild([this, inspector](Entity &currChild) {
			Entity::Name *childName = currChild.GetComponent<Entity::Name>();
			if (childName == nullptr)
			{
				// Skip bc it's not renderable
				return;
			}
			GenerateEntitySelectableTree(currChild, *childName, inspector);
		});
		ImGui::TreePop();
	}
}
