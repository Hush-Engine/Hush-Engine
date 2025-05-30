/*! \file Reflection.test.hpp
	\author Alan Ramirez
	\date 2025-05-25
	\brief Reflection test implementation
*/
#pragma once

#include "serialization/Formats/JsonSerializer.hpp"

#include <reflection/Type.hpp>
#include <Hushgen.hpp>

#if __has_include("Reflection.test.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "Reflection.test.hushgen.hpp"
#endif

struct [[hush::reflect]] AutogenTest
{
	HUSH_GENERATED_BODY
public:
	[[hush::property]]
	int myCustomField{0};

	[[hush::property]]
	float myFloatField{0.0f};
};