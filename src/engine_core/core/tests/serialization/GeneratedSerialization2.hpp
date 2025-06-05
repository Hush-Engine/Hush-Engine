/*! \file GeneratedSerialization2.hpp
	\author Alan Ramirez
	\date 2025-05-
	\brief Auto-generated serialization code for testing purposes
*/
#pragma once

#include "GeneratedSerialization.hpp"

#include <reflection/Type.hpp>
#include <Hushgen.hpp>

#if __has_include("GeneratedSerialization2.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "GeneratedSerialization2.hushgen.hpp"
#endif

struct [[hush::reflect]] SerializationAutogenStruct2
{
	HUSH_GENERATED_BODY
public:
	SerializationAutogenStruct2() = default;

	[[hush::property]]
	SerializationAutogenStruct field;
};