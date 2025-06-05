/*! \file Variant.cpp
	\author Alan Ramirez
	\date 2025-05-30
	\brief Variant implementation
*/

#include "Variant.hpp"

Hush::Reflection::VariantView::VariantView(const Variant &variant)
	: m_typeId(variant.m_typeId)
{
	if (variant.m_status == Variant::EVariantStatus::Small)
	{
		m_value = reinterpret_cast<void *>(const_cast<char *>(variant.m_data));
	}
	else if (variant.m_status == Variant::EVariantStatus::Large)
	{
		m_value = variant.m_ptr;
	}
	else
	{
		m_value = nullptr;
	}
}

Hush::Reflection::Variant::Variant(Variant &&rhs) noexcept
	: m_dtor(std::exchange(rhs.m_dtor, nullptr)),
	  m_status(std::exchange(rhs.m_status, EVariantStatus::None)),
	  m_typeId(std::exchange(rhs.m_typeId, TypeId{}))
{
	std::memcpy(m_data, rhs.m_data, sizeof(m_data));
}

Hush::Reflection::Variant::~Variant()
{
	Clear();
}