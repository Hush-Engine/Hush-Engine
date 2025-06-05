/*! \file GeneratedSerialization.hpp
	\author Alan Ramirez
	\date 2025-05-
	\brief Auto-generated serialization code for testing purposes
*/
#pragma once

#include <reflection/Type.hpp>
#include <Hushgen.hpp>

#if __has_include("GeneratedSerialization.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "GeneratedSerialization.hushgen.hpp"
#endif

struct [[hush::reflect]] SerializationAutogenStruct
{
	HUSH_GENERATED_BODY
public:
	SerializationAutogenStruct() = default;

	uint32_t GetValue1() const noexcept
	{
		return m_value1;
	}

	uint32_t GetValue2() const noexcept
	{
		return m_value2;
	}

	void SetValue1(uint32_t value) noexcept
	{
		m_value1 = value;
	}

	void SetValue2(uint32_t value) noexcept
	{
		m_value2 = value;
	}

private:
	[[hush::property]]
	uint32_t m_value1 = 0;

	[[hush::property]]
	uint32_t m_value2 = 0;
};