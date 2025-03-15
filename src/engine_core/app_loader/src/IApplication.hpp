/*! \file IApplication.hpp
	\author Alan Ramirez
	\date 2024-09-21
	\brief Application meant to be run by Hush
*/

#pragma once

#include "Scene.hpp"
#include <string>
#include <string_view>

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

		virtual void Init() = 0;

		virtual void Update(float delta) = 0;

		virtual void FixedUpdate(float delta) = 0;

		virtual void OnPreRender() = 0;

		virtual void OnRender() = 0;

		virtual void OnPostRender() = 0;

		virtual Hush::Scene *GetScene() = 0;

		[[nodiscard]]
		virtual std::string_view GetAppName() const noexcept = 0;
	};
} // namespace Hush
