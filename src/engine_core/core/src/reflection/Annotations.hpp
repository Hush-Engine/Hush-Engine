/*! \file Annotations.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Declarations used only as hush annotation arguments
*/

#pragma once

#include <cstddef>
#include <cstdint>

// The functions in this file are never called. They only exist so the
// arguments of the hush attributes are valid C++ expressions that the
// hush-reflection tool can read while parsing the headers.

namespace Hush::Reflection
{
	/// Overrides the canonical name of a reflected type.
	/// Used as: [[hush::reflect(Hush::Reflection::name("My.Game.Type"))]]
	template <std::size_t N>
	constexpr void name(const char (&)[N])
	{
	}

	/// Sets the update order of a system, between 0 and 255.
	/// Used as: [[hush::system(Hush::Reflection::order(10))]]
	constexpr void order(std::uint16_t)
	{
	}

	/// Sets a custom getter for a reflected property.
	/// Used as: [[hush::property(Hush::Reflection::Getter(&MyClass::GetValue))]]
	template <typename T>
	constexpr void Getter(T)
	{
	}

	/// Sets a custom setter for a reflected property.
	/// Used as: [[hush::property(Hush::Reflection::Setter(&MyClass::SetValue))]]
	template <typename T>
	constexpr void Setter(T)
	{
	}
} // namespace Hush::Reflection
