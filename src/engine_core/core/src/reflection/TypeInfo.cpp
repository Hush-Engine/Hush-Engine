#include "TypeInfo.hpp"

// Implementation file needed due to the export bindings

void Hush::Reflection::TypeInfo::SetAlignment(size_t alignment)
{
	this->m_alignment = alignment;
}

void Hush::Reflection::TypeInfo::SetSize(size_t size)
{
	this->m_size = size;
}

void Hush::Reflection::TypeInfo::SetName(std::string_view name)
{
	this->m_name = std::string(name.data(), name.size());
}
