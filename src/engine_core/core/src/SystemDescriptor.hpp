/*! \file SystemDescriptor.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Description of a system that a module can create
*/

#pragma once

#include "reflection/ModuleHandle.hpp"
#include "reflection/TypeId.hpp"

#include <cstdint>

namespace Hush
{
	class Scene;
	class ISystem;

	/// Describes a native system that belongs to a module. The reflection
	/// generator creates one of these for every class marked with
	/// [[hush::system]].
	struct SystemDescriptor
	{
		/// Type id of the system, the hash of its canonical name.
		Reflection::TypeId typeId;

		/// Canonical name of the system, for example "MyGame.TrafficSystem".
		const char *name;

		/// Update order of the system, between 0 and 255.
		std::uint16_t order;

		/// Creates a new system instance. The caller owns the result.
		ISystem *(*create)(Scene &scene);
	};
} // namespace Hush
