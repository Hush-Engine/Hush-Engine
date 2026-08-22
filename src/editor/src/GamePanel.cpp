#include "GamePanel.hpp"


void Hush::GamePanel::OnRender(float deltaTime) {
	(void)deltaTime;
}

void Hush::GamePanel::Init(Scene *activeScene) noexcept {
	this->m_activeScene = activeScene;
}

