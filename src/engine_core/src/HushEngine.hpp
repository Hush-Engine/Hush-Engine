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

#include <memory_resource>
#include <span>
#include <string_view>
#include <SDL3/SDL_events.h>
#include <memory>

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

		/// Returns a pointer to the memory resource used for frame-scoped allocations.
		///
		/// This memory resource is for temporary allocations that will live only for the duration of this frame.
		/// It resets at the end of each frame, allowing for efficient reuse of memory without fragmentation.
		///
		/// @note This is thread-safe. Each thread that calls this function
		///       will receive a pointer to a thread-local memory resource managed by the engine.
		///       All the created memory resources will be destroyed when the engine is destroyed.
		///
		/// @return A pointer to the frame scope memory resource.
		std::pmr::memory_resource *GetFrameScopeMemoryResource() noexcept;

		/// Returns a pointer to the memory resource used for scene-scoped allocations.
		///
		/// This memory resource is for allocations that should persist for the duration of a scene.
		/// It resets when a new scene is loaded, allowing for efficient reuse of memory without fragmentation across
		/// scenes.
		///
		/// @return A pointer to the scene scope memory resource.
		std::pmr::memory_resource *GetSceneScopeAllocator() noexcept;

		/// Rewinds the scene-scoped memory resource, reclaiming everything allocated from it.
		///
		/// Called when a scene is torn down (see `Scene::~Scene`). Must only be called once the
		/// scene that owns those allocations is gone, so no live object still references them.
		void ResetSceneScopeMemory() noexcept;

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
