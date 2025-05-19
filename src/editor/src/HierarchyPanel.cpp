#include "HierarchyPanel.hpp"
#include <imgui/imgui.h>
#include <Assertions.hpp>
#include <optional>
#include <string_view>
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "InspectorPanel.hpp"
#include "Logger.hpp"
#include "UI.hpp"

constexpr ImGuiWindowFlags DOCK_BASE_FLAGS =
	ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
	ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

void Hush::HierarchyPanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;
}

void Hush::HierarchyPanel::OnRender()
{
	ImGuiViewport *mainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowViewport(mainViewport->ID);
	ImGui::Begin("Hierarchy");
	auto &inspectorPanel = UI::Get().GetPanel<InspectorPanel>();
	Query<WorldTransform, LocalTransform> allEntities =
		this->m_activeScene->CreateQuery<WorldTransform, LocalTransform>();
	allEntities.Each([&inspectorPanel](Entity &entity, WorldTransform &_, LocalTransform &localxForm) {
		bool selected = inspectorPanel.GetInspectTarget().has_value() &&
						inspectorPanel.GetInspectTarget()->GetId() == entity.GetId();
		// Get the selectable that's related to the parent
		if (localxForm.HasParent())
		{
			// localxForm.GetParentId();
		}
		const std::optional<std::string_view> &name = entity.GetName();
		if (!ImGui::Selectable(name.value().data(), selected))
		{
			return;
		}
		inspectorPanel.SetInspectTarget(entity.GetId());
	});

	ImGui::End();
}
