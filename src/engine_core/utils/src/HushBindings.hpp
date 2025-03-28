/*! \file HushBindings.hpp
	\author Alan Ramirez Herrera
	\date 2024-02-01
	\brief Hush Engine bindings utils
*/

// clang-format off
// clang-tidy: off

#pragma once

#include <cstddef>

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
	}
}

// clang-format on
// clang-tidy: on
