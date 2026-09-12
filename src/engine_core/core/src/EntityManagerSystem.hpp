#pragma once

#include "ISystem.hpp"
#include "Query.hpp"
namespace Hush
{
	// Small component tag that will trigger deletion on this system
	struct EntityMarkedForDeletion {};

	class EntityManagerSystem final : public ISystem
	{
	public:

		using ISystem::ISystem;

		/// Init() is called when the system is initialized.
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

		/// GetName() is used to get the name of the system
		/// @return Name of the system
		[[nodiscard]]
		std::string_view GetName() const override {
			return "EntityManagerSystem";
		}


		/// @brief This system should be our very last one to run (at least the last one to interact with entity stuff)
		[[nodiscard]]
		std::uint16_t Order() const
		{
			return MAX_ORDER;
		}
		
	private:
		Query<EntityMarkedForDeletion> m_markedForDeletion;
	};

} // namespace Hush
