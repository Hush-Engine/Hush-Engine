/*! \file CFile.cpp
	\author Alan Ramirez
	\date 2024-12-22
	\brief C File implementation
*/

#include "CFile.hpp"

#include <Logger.hpp>
#include <cstdio>
Hush::CFile::~CFile()
{
	Close();
}

Hush::IFile::Result<unsigned long long> Hush::CFile::Read(std::span<std::byte> data)
{
	// if (this->m_file == nullptr) {
	// 	this->m_file = fopen(const char *FileName, const char *Mode)
	// }
	if (const auto read = fread(data.data(), sizeof(std::byte), data.size(), m_file); read != data.size())
	{
		return EError::CannotRead;
	}

	return data.size();
}
Hush::IFile::Result<std::span<std::byte>> Hush::CFile::Read(std::size_t size)
{
	(void)size;
	return EError::OperationNotSupported;
}

Hush::IFile::Result<void> Hush::CFile::Write(std::span<const std::byte> data)
{
	if (const auto written = fwrite(data.data(), sizeof(std::byte), data.size(), m_file); written != data.size())
	{
		return EError::CannotWrite;
	}

	return Success();
}

Hush::IFile::Result<void> Hush::CFile::Seek(std::size_t position)
{
	if (fseek(m_file, static_cast<long>(position), SEEK_SET) != 0)
	{
		return EError::OperationNotSupported;
	}

	return Success();
}

void Hush::CFile::Close()
{
	if (m_file != nullptr)
	{
		fclose(m_file);
		m_file = nullptr;
	}
}
