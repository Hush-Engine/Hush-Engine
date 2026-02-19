/*! \file HushEngine.hpp
	\author Kyn21kx
	\date 2024-02-28
	\brief Main class to instance our hush game engine
*/

#pragma once
#include "IApplication.hpp"
#include "ISystem.hpp"
#include "HushBindings.hpp"
#include "WindowRenderer.hpp"
#include "executors/ThreadPool.hpp"

#include <string_view>

namespace Hush
{
	struct DirectionalLight;

	class [[hush::export(Hush::Export::asHandle)]] HushEngine
	{
	public:
		/// Initializes the HushEngine with all its properties
		HushEngine();

		HushEngine(const HushEngine &) = delete;
		HushEngine &operator=(const HushEngine &) = delete;

		HushEngine(HushEngine &&) noexcept = delete;

		HushEngine &operator=(HushEngine &&) noexcept = delete;

		/// @brief Intended to be used for engine systems, user-defined systems should be added using
		/// `Hush::Scene::AddSystem()`
		void AddSystem(ISystem *system);

		~HushEngine();

		/// Starts running the engine with UI components
		void Run();

		/// Disposes of the HushEngine
		void Quit();

		[[hush::export]]
		Scene *GetScene();

		/// Returns the engine's default thread pool.
		/// The default threadpool contains a number of threads equal to the number of hardware threads available on the
		/// system, and each thread is pinned to a core.
		///
		/// @return A pointer to the engine's thread pool.
		[[nodiscard]]
		Threading::Executors::ThreadPool *GetEngineThreadPool() noexcept
		{
			return &m_threadPool;
		}

		Hush::WindowRenderer *GetWindowRenderer() noexcept
		{
			return m_windowRenderer.get();
		}

	private:
		void Init();

		std::unique_ptr<IApplication> m_app;
		std::unique_ptr<WindowRenderer> m_windowRenderer;
		Threading::Executors::ThreadPool m_threadPool;

		DirectionalLight *m_defaultLight = nullptr;
		bool m_isApplicationRunning = false;
		static constexpr std::string_view ENGINE_WINDOW_NAME = "Hush Engine";
	};

	/// Loads an application. The method to load an application depends on each platform and if shared library loading
	/// is enabled.
	///
	/// If HUSH_STATIC_APP definition is set to true, Hush won't try to load an application hosted in a shared library.
	/// This only applies on platforms that support shared libraries.
	///
	/// If a static application is bundled with the engine, it won't attempt to load a shared library.
	///
	/// @return A pointer to the loaded application.
	std::unique_ptr<IApplication> LoadApplication(HushEngine *engine);

} // namespace Hush
