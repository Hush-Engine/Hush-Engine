#pragma once

#include "IEditorPanel.hpp"

namespace Hush
{
	class GamePanel final : public IEditorPanel
	{
	public:
		void OnRender(float deltaTime) override;

		void Init(Scene *activeScene) noexcept override;

	private:
		Scene* m_activeScene = nullptr;

	};
} // namespace Hush
