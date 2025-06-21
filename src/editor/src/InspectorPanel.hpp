#pragma once

#include "Entity.hpp"
#include "IEditorPanel.hpp"
#include "imguizmo/ImGuizmo.h"
#include <optional>

namespace Hush
{
	class InspectorPanel final : public IEditorPanel
	{
	public:
		void OnRender() override;

		void Init(Scene *activeScene) noexcept override;

		void SetInspectTarget(Entity::EntityId entity);

		[[nodiscard]]
		const std::optional<Entity> &GetInspectTarget() const;

		[[nodiscard]]
		std::optional<Entity> &GetInspectTarget();

	private:
		void RenderProperties();

		std::optional<Entity> m_inspectTarget = std::nullopt;

		Scene *m_activeScene;

		ImGuizmo::OPERATION m_currentGizmoOp = ImGuizmo::OPERATION::TRANSLATE;
	};
} // namespace Hush
