/*! \file HierarchyPanel.hpp
	\author Kyn21kx
	\date 2024-05-24
	\brief Panel to display objects loaded to a scene
*/

#pragma once

#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "IEditorPanel.hpp"
#include "Query.hpp"

namespace Hush
{
	class HierarchyPanel final : public IEditorPanel
	{
	public:
		void Init(Scene *activeScene) noexcept override;

		void OnRender(float deltaTime) override;

	private:
		Scene *m_activeScene;
		Query<WorldTransform, LocalTransform, Entity::Name> m_inspectableEntitiesQuery;
	};
} // namespace Hush
