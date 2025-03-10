#pragma once

#include "Entity.hpp"
#include "IEditorPanel.hpp"

namespace Hush {
	class InspectorPanel final : public IEditorPanel {
	public:
		void OnRender() override;

		void Init(Scene* activeScene) noexcept override;

		void SetInspectTarget(Entity* entity);
	private:
		void RenderProperties();
		
		Entity* m_inspectTarget;
	}
}

