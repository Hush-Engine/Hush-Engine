/*! \file GeneratedModuleSystem.hpp
	\brief System used to verify the generated native module entry point
*/

#pragma once

#include "ISystem.hpp"

#include <reflection/Annotations.hpp>
#include <reflection/Type.hpp>
#include <serialization/Deserialization.hpp>
#include <serialization/Serialization.hpp>
#include <Hushgen.hpp>

#if __has_include("GeneratedModuleSystem.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "GeneratedModuleSystem.hushgen.hpp"
#endif

namespace Hush::Tests
{
	class [[hush::system(Hush::Reflection::order(17))]] GeneratedModuleSystem final : public ISystem
	{
		HUSH_GENERATED_BODY

	public:
		explicit GeneratedModuleSystem(Scene &scene)
			: ISystem(scene)
		{
		}

		void Init() override;
		void OnShutdown() override;
		void OnUpdate(float delta) override;
		void OnFixedUpdate(float delta) override;
		void OnRender() override;
		void OnPreRender() override;
		void OnPostRender() override;

		[[nodiscard]]
		std::string_view GetName() const override;
	};
} // namespace Hush::Tests
