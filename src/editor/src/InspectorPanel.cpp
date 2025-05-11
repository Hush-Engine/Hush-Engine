#include "InspectorPanel.hpp"
#include "Assertions.hpp"
#include "Components/WorldTransform.hpp"
#include "Logger.hpp"
#include "Shared/IMaterial3D.hpp"
#include "Vulkan/GltfMetallicRoughness.hpp"
#include "imgui/imgui.h"
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <optional>
#include <string>
#include <vector>
#include "UI.hpp"
#include "Shared/DirectionalLight.hpp"

#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "Shared/Mesh.hpp"

constexpr float NESTED_INDENT_SIZE = 10.0F;

void Hush::Serialize(DirectionalLight *component)
{
	ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen);
	glm::vec4 &rgba = component->color.GetRGBA32F();
	auto *rgbRegion = reinterpret_cast<float *>(&rgba);
	ImGui::ColorEdit3("Light Color", rgbRegion);
	ImGui::InputFloat("Intensity", &component->intensity);
}

void Hush::Serialize(IMaterial3D *component)
{
	// ECullMode cullMode = component->GetCullMode();
	// Check which instance of the material is
	// TODO: Do this with the reflection API instead of dynamic casting
	auto *pbrMaterial = dynamic_cast<GLTFMetallicRoughness *>(component);
	if (pbrMaterial == nullptr)
	{
		return;
	}
	const float range = 10.0F;

	// Albedo color
	glm::vec4 &albedo = pbrMaterial->GetAlbedo();
	ImGui::ColorEdit4("Albedo", reinterpret_cast<float *>(&albedo));

	glm::vec3 &emission = pbrMaterial->GetEmissionColor();
	ImGui::ColorEdit3("Emission", reinterpret_cast<float *>(&emission));

	// TODO: Turn the float setters into references (try to reconcile this with CTRL + Z)
	
	float emissionFactor = pbrMaterial->EmissionFactor();
	ImGui::SliderFloat("Emission factor", &emissionFactor, -range, range);
	
	pbrMaterial->SetEmissionFactor(emissionFactor);

	float roughness = pbrMaterial->GetRoughnessFactor();

	ImGui::SliderFloat("Roughness factor", &roughness, 0.F, 1.0F);
	pbrMaterial->SetRoughnessFactor(roughness);

	float metallic = pbrMaterial->GetMetallicFactor();
	ImGui::SliderFloat("Metallic factor", &metallic, -1.0F, 1.0F);
	pbrMaterial->SetMetallicFactor(metallic);
}

void Hush::Serialize(Mesh *component)
{
	if (!ImGui::CollapsingHeader("Mesh Component", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	// TODO: Maybe write this as a table
	ImGui::Indent(NESTED_INDENT_SIZE);
	const std::vector<GeoSurface> &surfaces = component->GetSurfaces();
	// Iterate over the surfaces and  serialize their materials as submeshes
	for (size_t i = 0; i < surfaces.size(); i++)
	{
		const GeoSurface& surface = surfaces[i];
		if (ImGui::CollapsingHeader((std::string("Surface") + std::to_string(i)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			Serialize(surface.material.get());
		}
	}
	ImGui::Unindent(NESTED_INDENT_SIZE);
}


void Hush::Serialize(Transform* component) 
{
	if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}

	glm::vec3* pos = component->GetPosition();
	glm::vec3 scale = component->GetScale();
	glm::vec3 rot = glm::degrees(component->GetEulerAngles());
	ImGui::Text("Position");
	ImGui::InputFloat3("##Position", reinterpret_cast<float *>(pos));

	ImGui::Text("Rotation");
	ImGui::InputFloat3("##Rotation", reinterpret_cast<float *>(&rot));

	ImGui::Text("Scale");
	ImGui::InputFloat3("##Scale", reinterpret_cast<float *>(&scale));
	if (scale != component->GetScale()) {
		component->SetScale(scale);
	}
	if (rot != glm::degrees(component->GetEulerAngles())) {
		component->SetRotationQuat(glm::quat(glm::radians(rot)));
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
	WorldTransform *transform = this->m_inspectTarget->GetComponent<WorldTransform>();
	HUSH_ASSERT(transform != nullptr, "Trying to render an entity without a Transform component!");
	Serialize(transform);
	// Get all the other entity's components
	// TODO: handle this with reflection
	DirectionalLight *dirLightComponent = this->m_inspectTarget->GetComponent<DirectionalLight>();
	if (dirLightComponent != nullptr)
	{
		Serialize(dirLightComponent);
	}

	Mesh *meshComponent = this->m_inspectTarget->GetComponent<Mesh>();
	if (meshComponent != nullptr)
	{
		Serialize(meshComponent);
	}
}
