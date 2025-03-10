#pragma once

#include "Entity.hpp"
#include "IEditorPanel.hpp"

namespace Hush {
	class InspectorPanel final : public IEditorPanel {
	public:
		void OnRender() override;

		void Init(Scene* activeScene) noexcept override;

		void SetInspectTarget(Entity::EntityId entity);
	private:
		void RenderProperties();
		
		std::optional<Entity> m_inspectTarget = std::nullopt;
		
		Scene* m_activeScene;
	};
}

