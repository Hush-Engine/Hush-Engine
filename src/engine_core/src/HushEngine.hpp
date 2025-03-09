/*! \file HushEngine.hpp
	\author Kyn21kx
	\date 2024-02-28
	\brief Main class to instance our hush game engine
*/

#pragma once
#include "IApplication.hpp"
#include "ISystem.hpp"

#include <string_view>

namespace Hush
{
	class HushEngine
	{
	public:
		/// <summary>
		/// Initializes the HushEngine with all its properties
		/// </summary>
		HushEngine() = default;

		HushEngine(const HushEngine &) = delete;
		HushEngine &operator=(const HushEngine &) = delete;

		HushEngine(HushEngine &&) noexcept = default;

		HushEngine &operator=(HushEngine &&) noexcept = default;

		/// @brief Intended to be used for engine systems, user-defined systems should be added using `Hush::Scene::AddSystem()`
		void AddSystem(ISystem* system);
		
		~HushEngine();

		/// <summary>
		/// Starts running the engine with UI components
		/// </summary>
		void Run();

		/// <summary>
		/// Disposes of the HushEngine
		/// </summary>
		void Quit();

	private:
		void Init();

		std::unique_ptr<IApplication> m_app;

		bool m_isApplicationRunning = false;
		static constexpr std::string_view ENGINE_WINDOW_NAME = "Hush Engine";
	};

} // namespace Hush
