#pragma once

#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "ISystem.hpp"
#include "Query.hpp"

namespace Hush
{
	class RenderingSystem final : public ISystem
	{
	public:
		using ISystem::ISystem;

		void Init() override;

		void OnShutdown() override;

		void OnUpdate(float delta) override;

		void OnFixedUpdate(float delta) override;

		void OnRender() override;

		void OnPreRender() override;

		void OnPostRender() override;

		[[nodiscard]]
		std::string_view GetName() const override;

	private:
		Query<const MeshReference, const WorldTransform> m_renderableTargetsQuery;
	};
} // namespace Hush
