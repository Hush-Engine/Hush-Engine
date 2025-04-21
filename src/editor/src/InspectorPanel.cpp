#include "InspectorPanel.hpp"
#include "Assertions.hpp"
#include "Components/Transform.hpp"
#include "Shared/IMaterial3D.hpp"
#include "Shared/Mesh.hpp"
#include "Vulkan/GltfMetallicRoughness.hpp"
#include "imgui/imgui.h"
#include <glm/ext/vector_float3.hpp>
#include <optional>
#include <vector>
#include "UI.hpp"
#include "Shared/DirectionalLight.hpp"

void Hush::Serialize(DirectionalLight *component)
{
	ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen);
	glm::vec4 &rgba = component->color.GetRGBA32F();
	auto *rgbRegion = reinterpret_cast<float *>(&rgba);
	ImGui::ColorEdit3("Light Color", rgbRegion);
	ImGui::InputFloat("Intensity", &component->intensity);
}

void Hush::Serialize(IMaterial3D* component) {
	// ECullMode cullMode = component->GetCullMode();
	// Check which instance of the material is
	// TODO: Do this with the reflection API instead of dynamic casting
	auto* pbrMaterial = dynamic_cast<GLTFMetallicRoughness*>(component);
	if (pbrMaterial == nullptr) {
		return;
	}
	
	// Albedo color
	glm::vec4& albedo = pbrMaterial->GetAlbedo();
	ImGui::ColorEdit4("Albedo", reinterpret_cast<float*>(&albedo));
	float metallic = pbrMaterial->GetMetallicFactor();
	ImGui::SliderFloat("Metallic factor", &metallic, -100.0F, 100.0F);
	pbrMaterial->SetMetallicFactor(metallic);
}


void Hush::Serialize(Mesh* component) {
	ImGui::CollapsingHeader("Mesh Component", ImGuiTreeNodeFlags_DefaultOpen);
	const std::vector<GeoSurface>& surfaces = component->GetSurfaces();
	// Iterate over the surfaces and  serialize their materials as submeshes
	for (const GeoSurface& surface : surfaces) {
		ImGui::CollapsingHeader("Surface", ImGuiTreeNodeFlags_DefaultOpen);
		Serialize(surface.material.get());
	}
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

std::optional<Hush::Entity> &Hush::InspectorPanel::GetInspectTarget()
{
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
	DirectionalLight *dirLightComponent = this->m_inspectTarget->GetComponent<DirectionalLight>();
	if (dirLightComponent != nullptr)
	{
		Serialize(dirLightComponent);
	}
	
	Mesh* meshComponent = this->m_inspectTarget->GetComponent<Mesh>();
	if (meshComponent != nullptr)
	{
		Serialize(meshComponent);
	}
	
}
