#include "StatsPanel.hpp"
#include "imgui/imgui.h"


void Hush::StatsPanel::Init(Scene* activeScene) noexcept {
	(void)activeScene;
}

void Hush::StatsPanel::OnRender() noexcept
{
	ImGui::Begin("Hush Engine Stats");
	ImGui::Text("Delta time: %.4fs", this->m_deltaTime);
	ImGui::Text("FPS: %.2f", this->m_framesPerSecond);
	ImGui::Text("Draw calls: %d", this->m_drawCallCount);
	ImGui::Text("GPU driver: %s", this->m_deviceName.c_str());
	ImGui::End();
}

void Hush::StatsPanel::SetDeltaTime(float delta)
{
	this->m_deltaTime = delta;
	this->m_framesPerSecond = 1.0f / delta;
}

void Hush::StatsPanel::SetDrawCallsCount(int32_t count)
{
	this->m_drawCallCount = count;
}

void Hush::StatsPanel::SetDeviceName(const std::string &deviceName)
{
	this->m_deviceName = deviceName;
}
