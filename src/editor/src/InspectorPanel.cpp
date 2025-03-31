#include "InspectorPanel.hpp"
#include "Assertions.hpp"
#include "Components/Transform.hpp"
#include "imgui/imgui.h"
#include <glm/ext/vector_float3.hpp>
#include <optional>
#include "UI.hpp"
#include "Shared/DirectionalLight.hpp"

void Hush::Serialize(DirectionalLight *component) {
	ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen);
	glm::vec4& rgba = component->color.GetRGBA32F();
	auto* rgbRegion = reinterpret_cast<float*>(&rgba);
	ImGui::ColorEdit3("Light Color", rgbRegion);
	ImGui::InputFloat("Intensity", &component->intensity);
}


void Hush::InspectorPanel::OnRender()
{
	ImGui::Begin("Inspector");
	this->RenderProperties();
	ImGui::End();
}

void Hush::InspectorPanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;
}

void Hush::InspectorPanel::SetInspectTarget(Entity::EntityId entity)
{
	this->m_inspectTarget = this->m_activeScene->EntityFromId(entity);
}

const std::optional<Hush::Entity> &Hush::InspectorPanel::GetInspectTarget() const
{
	return this->m_inspectTarget;
}


std::optional<Hush::Entity> &Hush::InspectorPanel::GetInspectTarget() {
	return this->m_inspectTarget;
}

void Hush::InspectorPanel::RenderProperties()
{
	if (!this->m_inspectTarget.has_value())
	{
		return;
	}
	ImGui::SeparatorText(this->m_inspectTarget->GetName().value_or("").data());
	Transform *transform = this->m_inspectTarget->GetComponent<Transform>();
	HUSH_ASSERT(transform != nullptr, "Trying to render an entity without a Transform component!");
	UI::SerializeComponent(transform, UI::ESerializableComponentType::Transform);
	// Get all the other entity's components
	// TODO: handle this with reflection
	DirectionalLight* dirLightComponent = this->m_inspectTarget->GetComponent<DirectionalLight>();
	if (dirLightComponent == nullptr) {
		return;
	}
	Serialize(dirLightComponent);
}
