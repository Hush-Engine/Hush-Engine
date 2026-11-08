/*! \file HushBindings.hpp
	\author Alan Ramirez Herrera
	\date 2024-02-01
	\brief Hush Engine bindings utils
*/
// NOLINTBEGIN

#pragma once

#include <cstddef>
#include "Platform.hpp"

#if HUSH_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 5030) // Attribute not recognized

#pragma warning(push)
#pragma warning(disable : 4068) // Unknown pragma
#endif

#if HUSH_COMPILER_CLANG
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunknown-attributes"
#elif HUSH_COMPILER_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-attributes"
#endif

namespace Hush::Export
{
	/// Export a class or struct as a handle. This will generate a typedef for the class or struct.
	/// If not defined, the class or struct will be exported as a struct with all the members.
	///
	/// For instance, given the following class:
	/// class [[hush::export(Hush::Export::asHandle)]] MyClass
	/// {
	/// public:
	///     int myVariable;
	/// };
	/// The generated C code will be:
	/// typedef struct MyClass MyClass;
	constexpr bool asHandle = true;

	/// Ignores a member of a class or struct. It, however, will be exported as a buffer.
	/// For instance, given the following class:
	/// class [[hush::export(Hush::Export::ignore)]] MyClass
	/// {
	/// public:
	///     int myVariable;
	///     [[hush::export(Hush::Export::ignore)]]
	///     int myVariable2;
	/// };
	/// The generated C code will be:
	/// typedef struct MyClass MyClass
	/// {
	///     int myVariable;
	///     alignas(4) char myVariable2[4];
	/// };
	constexpr bool ignore = true;

	// Helpful functions for template generation


	/// @brief Produces a macro marker for a symbol that specifies parameters that could be converted to templated component ID searches at runtime
	/// @details A marker macro output for this function that comes from the input
	/// @code {.cpp}
	/// // (On Entity.h)
	/// [[hush::export(Hush::Export::queryCompIdParams("componentId"))]]
	/// bool RemoveComponentRaw(EntityId componentId);
	/// @endcode
	/// Would produce:
	/// @code {.c}
	/// HUSH_QCOMP_ID(T_PAIR(T0, "componentId"))
	/// extern bool Hush__Entity__RemoveComponentRaw(Hush__Entity *self, Hush__Entity_EntityId componentId);
	/// @endcode
	/// The binding generator's job is then to process that macro into a wrapper function at its target file
	/// @code {.cs}
	/// public bool RemoveComponent<T0>() {
	///    return this.RemoveComponentRaw(QueryCompId<T0>());
	/// }
	/// @endcode
	/// Where the query function can do a hash lookup based on T0's typename, an array lookup, whatever the language seems fit
	template <std::size_t N>
	constexpr void queryCompIdParams(const char(&names)[N] ...) {
		(void)names;
	}

	template <std::size_t N>
	constexpr void typeNameParams(const char(&names)[N] ...) {
		(void)names;
	}

	template <std::size_t N>
	constexpr void templatedReturn(const char(&typeSpecifier)[N]) { // i.e templatedReturn("T0")
		(void)typeSpecifier;
	}

	template <std::size_t N>
	constexpr void typeSizeParams(const char(&typeSpecifiers)[N]...) {
		(void)typeSpecifiers;
	}
	

	/// Renames a class or struct.
	/// @param name The new name of the class or struct.
	/// For instance, given the following class:
	/// class [[hush::export(Hush::Export::name("MyExportedClass"))]] MyClass2
	/// {
	/// public:
	///     int myVariable;
	/// };
	/// The generated C code will be:
	/// typedef struct MyExportedClass MyExportedClass;
	template <std::size_t N>
	constexpr void name(const char (&name)[N])
	{
		// Do nothing
		(void)name;
	}
} // namespace Hush::Export

#if HUSH_COMPILER_MSVC
#pragma warning(pop)
#endif

// NOLINTEND
