/*! \file HushEngine.hpp
	\author Kyn21kx
	\date 2024-02-28
	\brief Main class to instance our hush game engine
*/

#pragma once
#include "IApplication.hpp"
#include "ISystem.hpp"
#include "HushBindings.hpp"
#include "executors/ThreadPool.hpp"

#include <span>
#include <string_view>
#include <SDL3/SDL_events.h>

namespace Hush
{
	class VirtualFilesystem;
	class ResourceManager;
	class WindowRenderer;

	class [[hush::export(Hush::Export::asHandle)]] HushEngine
	{
		/// Forward declaration of the internal implementation class, which is hidden from users of the engine.
		/// This allows us to hide implementation details, reduce compile-time dependencies, and
		/// having the freedom to change the internal data without affecting the ABI of this class.
		struct HushEngineInternal;

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

		void Init(int argc, char **argv);

		/// Starts running the engine with UI components
		void Run();

		/// Disposes of the HushEngine
		void Quit();

		void HandleEvents(const SDL_Event &event);

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

		Hush::WindowRenderer *GetWindowRenderer() noexcept;

		VirtualFilesystem *GetVirtualFilesystem() noexcept;

		ResourceManager *GetResourceManager() noexcept;

	private:
		void AddDefaultSystems();

		std::unique_ptr<IApplication> m_app = nullptr;
		std::unique_ptr<HushEngineInternal> m_internal = nullptr;
		Threading::Executors::ThreadPool m_threadPool;

		std::chrono::steady_clock::duration m_elapsed;

		bool m_isApplicationRunning = false;
		static constexpr std::string_view ENGINE_WINDOW_NAME = "Hush Engine";
	};
} // namespace Hush
