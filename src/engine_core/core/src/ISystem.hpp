/*! \file ISystem.hpp
	\author Alan Ramirez
	\date 2025-01-15
	\brief ISystem implementation
*/

#pragma once

#include <cstdint>
#include <string_view>

namespace Hush
{
	class Scene;

	/// ISystem is the base interface for all systems in the engine
	/// It implements helper functions that all systems should implement.
	///
	/// The lifecycle of a system is as follows:
	/// - Constructor: Called when the system is created, could be used for some early initialization
	/// - Init: Called when the system is initialized, at this point all the systems are created.
	/// - OnUpdate: Called every frame, this is where the system should update its state.
	/// - OnFixedUpdate: Called every fixed frame, this is where the system should update its state. Usually helpful for
	/// physics.
	/// - OnShutdown: Called when the system is shutting down, could be used for some cleanup.
	/// - OnRender: Called when the system should render, could be used for rendering.
	/// - Destructor: Called when the system is destroyed, could be used for some cleanup.
	///
	/// Hooks are provided for all the lifecycle events, so the system can implement them as needed.
	///
	/// Systems provide a member function called `Order` that should return the order in which the system should be
	/// updated. The game engine will sort the systems based on this value. Every time a new system is added, the engine
	/// will sort the systems based on this value. The engine provides 255 order slots, so the system can return any
	/// value between 0 and 255. Systems that belong to the same order will be updated concurrently.
	///
	/// The systems will be bucketed BEFORE init is called, so the order of the systems should be set in the
	/// constructor.
	class ISystem
	{
	public:
		static constexpr std::uint16_t MAX_ORDER = 255;
		
		ISystem(Scene &scene)
			: m_scene(&scene)
		{
		}

		ISystem(const ISystem &) = delete;
		ISystem &operator=(const ISystem &) = delete;
		ISystem(ISystem &&) noexcept = default;
		ISystem &operator=(ISystem &&) noexcept = default;

		virtual ~ISystem() = default;

		/// Init() is called when the system is initialized.
		virtual void Init() = 0;

		/// OnShutdown() is called when the system is shutting down.
		virtual void OnShutdown() = 0;

		/// OnRender() is called when the system should render.
		/// @param delta Time since last frame
		virtual void OnUpdate(float delta) = 0;

		/// OnFixedUpdate() is called when the system should update its state.
		/// @param delta Time since last fixed frame
		virtual void OnFixedUpdate(float delta) = 0;

		/// OnRender() is called when the system should render.
		virtual void OnRender() = 0;

		/// OnPreRender() is called before rendering.
		virtual void OnPreRender() = 0;

		/// OnPostRender() is called after rendering.
		virtual void OnPostRender() = 0;

		/// Order() is used to get the order in which the system should be updated
		/// @return Order in which the system should be updated
		[[nodiscard]]
		std::uint16_t Order() const
		{
			return m_order;
		}

		/// GetName() is used to get the name of the system
		/// @return Name of the system
		[[nodiscard]]
		virtual std::string_view GetName() const = 0;

		[[nodiscard]]
		Scene &GetScene() const
		{
			return *m_scene;
		}

	protected:
		/// SetOrder() is used to set the order in which the system should be updated
		/// @param order Order in which the system should be updated. If the order is greater than MAX_ORDER, it will be
		///              set to MAX_ORDER.
		void SetOrder(std::uint16_t order)
		{
			m_order = order > MAX_ORDER ? MAX_ORDER : order;
		}

	private:
		std::uint16_t m_order = 0;
		Scene *m_scene = nullptr;
	};
} // namespace Hush
