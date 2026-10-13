/*! \file IApplication.hpp
	\author Alan Ramirez
	\date 2024-09-21
	\brief Application meant to be run by Hush
*/

#pragma once

#include "Entity.hpp"
#include "Scene.hpp"
#include <string_view>
#include <filesystem>

namespace Hush
{
	class IApplication
	{
	public:
		IApplication() = default;

		IApplication(const IApplication &) = delete;
		IApplication(IApplication &&) = delete;
		IApplication &operator=(const IApplication &) = default;
		IApplication &operator=(IApplication &&) = default;

		virtual ~IApplication() = default;

		// This should scan all relevant module paths and find their dlls, maybe a default implmenation would be good(?
		virtual void LoadModules(const std::filesystem::path &modulesRoot) = 0;

		virtual void Init() = 0;

		virtual void Update(float delta) = 0;

		virtual void FixedUpdate(float delta) = 0;

		virtual void OnPreRender() = 0;

		virtual void OnRender(float delta) = 0;

		virtual void OnPostRender() = 0;

		virtual Hush::Scene *GetScene() = 0;

		// Temp workaround until we figure out engine system ordering
		virtual void DisposeFrame() = 0;

		[[nodiscard]]
		virtual std::string_view GetAppName() const noexcept = 0;
	};
} // namespace Hush
