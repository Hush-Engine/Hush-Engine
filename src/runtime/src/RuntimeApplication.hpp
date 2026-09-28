/*! \file RuntimeApplication.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Generic runtime player for exported Hush projects
*/

#pragma once

#include "IApplication.hpp"
#include "RuntimeManifest.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

namespace Hush
{
	class HushEngine;

	/// Generic application that runs an exported project: it mounts the
	/// cooked content, loads the gameplay modules and creates the configured
	/// systems. Projects do not need their own C++ application class.
	class RuntimeApplication final : public IApplication
	{
	public:
		/// Name of the manifest file, searched next to the executable.
		static constexpr std::string_view DEFAULT_MANIFEST_NAME = "hush-runtime.json";

		explicit RuntimeApplication(HushEngine *engine);

		RuntimeApplication(const RuntimeApplication &) = delete;
		RuntimeApplication(RuntimeApplication &&) = delete;
		RuntimeApplication &operator=(const RuntimeApplication &) = delete;
		RuntimeApplication &operator=(RuntimeApplication &&) = delete;

		~RuntimeApplication() override = default;

		/// Loads the manifest, mounts the content, loads the modules and
		/// creates the configured systems.
		void Init() override;

		void Update(float delta) override;

		void FixedUpdate(float delta) override;

		void OnPreRender() override;

		void OnRender(float delta) override;

		void OnPostRender() override;

		void DisposeFrame() override;

		[[nodiscard]]
		Hush::Scene *GetScene() override;

		[[nodiscard]]
		std::string_view GetAppName() const noexcept override;

	private:
		/// Loads every module listed in the manifest.
		void LoadModules(const RuntimeManifest &manifest, const std::filesystem::path &manifestDir);

		/// Creates every system listed in the manifest and adds it to the scene.
		void CreateSystems(const RuntimeManifest &manifest);

		HushEngine *m_engine;
		std::unique_ptr<Scene> m_scene;
		std::string m_appName = "Hush Runtime";
	};
} // namespace Hush
