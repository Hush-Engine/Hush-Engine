/*! \file HierarchyPanel.hpp
	\author Kyn21kx
	\date 2024-05-24
	\brief Panel to display objects loaded to a scene
*/

#pragma once

#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "Entity.hpp"
#include "IEditorPanel.hpp"
#include "Query.hpp"
#include <array>
#include <unordered_set>

namespace Hush
{
	class InspectorPanel;
	class HierarchyPanel final : public IEditorPanel
	{
	public:
		void Init(Scene *activeScene) noexcept override;

		void OnRender(float deltaTime) override;

	private:
		enum class EState {
			None = 0,
			DeletePopupOpen,
			Renaming
		};


		// NOTE: Maybe centralize this into an EditorInput file or something
		void HandleInput();

		Scene *m_activeScene;
		ComponentRef m_editorInfo;
		Query<WorldTransform, LocalTransform, Entity::Name> m_inspectableEntitiesQuery;
		EState m_state = EState::None;
		std::array<char, Entity::MAX_ENTITY_NAME_LENGTH> m_entityRenameBuffer{};

		// Small helper WITH recursion
		// PERF: Remove recursion from this function
		void GenerateEntitySelectableTree(Entity &entity, const Entity::Name &name, InspectorPanel *inspector);
	};
} // namespace Hush
