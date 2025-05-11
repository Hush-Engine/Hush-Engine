#pragma once

#include "Transform.hpp"
#include "../Entity.hpp"

namespace Hush
{
	struct LocalTransform : public Transform
	{
		using Transform::Transform;

	public:
		[[nodiscard]]
		const Entity::EntityId &GetParentId() const
		{
			return this->m_parentId;
		}

		void SetParent(const Entity::EntityId &parent)
		{
			this->m_parentId = parent;
			this->m_hasParent = true;
		}

		[[nodiscard]]
		bool HasParent() const noexcept
		{
			return this->m_hasParent;
		}

	private:
		Entity::EntityId m_parentId{};
		bool m_hasParent = false;
	};
} // namespace Hush
