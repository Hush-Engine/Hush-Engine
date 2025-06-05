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
	[[hush::function]]
	AutogenTest()
	{
	}

	[[hush::function]]
	explicit AutogenTest(int field)
		: myCustomField(field)
	{
	}

	[[hush::function]]
	void SetTo10()
	{
		this->myCustomField = 10;
	}

	[[hush::function]]
	void SetTo(int value)
	{
		this->myCustomField = value;
	}

	[[hush::function]]
	void GetFromRef(int &outValue) const
	{
		outValue = myCustomField;
	}

	[[hush::property]]
	int myCustomField{0};

	[[hush::property]]
	float myFloatField{0.0f};

	int otherField{5};
};