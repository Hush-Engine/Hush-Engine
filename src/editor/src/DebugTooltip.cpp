#include "DebugTooltip.hpp"
#include "imgui/imgui.h"

void Hush::DebugTooltip::OnRender([[maybe_unused]] float deltaTime) noexcept
{
	if (s_debugTooltip == nullptr)
	{
		s_debugTooltip = this;
	}
	ImGui::Begin("Debug tooltip (for rendering)");
	float vecArr[] = {this->m_scale.x, this->m_scale.y, this->m_scale.z};
	ImGui::SliderFloat3("Set the scale offset", vecArr, -1.0f, 1.0f);

	this->m_scale = *reinterpret_cast<glm::vec3 *>(vecArr);

	ImGui::End();
}

const glm::vec3 &Hush::DebugTooltip::GetScale() const
{
	return this->m_scale;
}

const glm::vec3 &Hush::DebugTooltip::GetRotation() const
{
	return this->m_rotation;
}

const glm::vec3 &Hush::DebugTooltip::GetTranslation() const
{
	return this->m_translation;
}
