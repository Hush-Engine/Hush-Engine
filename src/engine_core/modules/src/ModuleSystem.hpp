/*! \file ModuleSystem.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief ISystem adapter for systems implemented in gameplay modules
*/

#pragma once

#include "HushModuleAbi.h"

#include "ISystem.hpp"
#include "reflection/ModuleHandle.hpp"
#include "reflection/TypeId.hpp"

#include <string>
#include <string_view>

namespace Hush::Modules
{
	/// Adapts a foreign language system object into a normal ISystem. The
	/// engine only ever sees this adapter, the foreign object stays owned by
	/// the module that created it.
	class ModuleSystem final : public ISystem
	{
	public:
		/// @param scene Scene the system belongs to.
		/// @param module Module that owns the foreign object.
		/// @param object Opaque handle of the foreign object.
		/// @param runtimeOps Callback table of the owning module. The table is
		/// copied, the module must stay loaded while the system is alive.
		/// @param name Canonical name of the system.
		/// @param order Update order of the system.
		/// @param lifecycleMask Bit mask of the lifecycle functions the
		/// foreign object implements.
		ModuleSystem(Scene &scene, ModuleHandle module, HushObjectHandle object, HushSystemRuntimeOps runtimeOps,
					 std::string_view name, std::uint16_t order, std::uint32_t lifecycleMask);

		/// Destroys the foreign object through the owning module callbacks.
		~ModuleSystem() override;

		ModuleSystem(const ModuleSystem &) = delete;
		ModuleSystem &operator=(const ModuleSystem &) = delete;
		ModuleSystem(ModuleSystem &&) = delete;
		ModuleSystem &operator=(ModuleSystem &&) = delete;

		void Init() override;

		void OnUpdate(float delta) override;

		void OnFixedUpdate(float delta) override;

		void OnShutdown() override;

		void OnPreRender() override;

		void OnRender() override;

		void OnPostRender() override;

		[[nodiscard]]
		std::string_view GetName() const override
		{
			return m_name;
		}

		/// The module that owns the foreign object.
		[[nodiscard]]
		ModuleHandle GetModule() const
		{
			return m_module;
		}

	private:
		ModuleHandle m_module;
		HushObjectHandle m_object;
		HushSystemRuntimeOps m_ops;
		std::string m_name;
		std::uint32_t m_lifecycleMask;
	};
} // namespace Hush::Modules
