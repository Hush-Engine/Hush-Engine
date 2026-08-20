#pragma once

#include "Entity.hpp"
#include "IEditorPanel.hpp"
#include <optional>
#include "components/EditorInfo.hpp"

namespace Hush
{
	class InspectorPanel final : public IEditorPanel
	{
	public:
		void OnRender(float deltaTime) override;

		void Init(Scene *activeScene) noexcept override;

		void SetInspectTarget(Entity::EntityId entity);

		[[nodiscard]]
		const std::optional<Entity> &GetInspectTarget() const;

		[[nodiscard]]
		std::optional<Entity> &GetInspectTarget();

	private:
		void RenderProperties();

		std::optional<Entity> m_inspectTarget = std::nullopt;

		EditorInfo *m_editorInfo = nullptr;

		Scene *m_activeScene;
	};
} // namespace Hush
