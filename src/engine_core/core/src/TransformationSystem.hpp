#pragma once

#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "ISystem.hpp"
#include "Query.hpp"

#include <reflection/Type.hpp>
#include <serialization/Deserialization.hpp>
#include <serialization/Serialization.hpp>
#include <Hushgen.hpp>

#if __has_include("TransformationSystem.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "TransformationSystem.hushgen.hpp"
#endif

namespace Hush
{

	// Takes care of all transformation calculations per entity
	// this also determines how entities are rendered since their global transform component is updated
	class [[hush::system]] TransformationSystem final : public ISystem
	{
		HUSH_GENERATED_BODY
	public:
		explicit TransformationSystem(Scene &scene)
			: ISystem(scene)
		{
		}

		void Init() override;

		/// OnShutdown() is called when the system is shutting down.
		void OnShutdown() override;

		/// OnRender() is called when the system should render.
		/// @param delta Time since last frame
		void OnUpdate(float delta) override;

		/// OnFixedUpdate() is called when the system should update its state.
		/// @param delta Time since last fixed frame
		void OnFixedUpdate(float delta) override;

		/// OnRender() is called when the system should render.
		void OnRender() override;

		/// OnPreRender() is called before rendering.
		void OnPreRender() override;

		/// OnPostRender() is called after rendering.
		void OnPostRender() override;

		[[nodiscard]]
		std::string_view GetName() const override
		{
			return "TransformationSystem";
		}

	private:
		Query<WorldTransform, LocalTransform> m_transformableEntitiesQuery;
	};
} // namespace Hush
