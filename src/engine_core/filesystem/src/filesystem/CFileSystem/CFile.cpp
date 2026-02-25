/*! \file CFile.cpp
	\author Alan Ramirez
	\date 2024-12-22
	\brief C File implementation
*/

#include "CFile.hpp"

#include <Logger.hpp>
#include <algorithm>
#include <cstdio>
Hush::CFile::~CFile()
{
	Close();
}

Hush::CFile::Result<std::size_t> Hush::CFile::Read(std::span<std::byte> data)
{
	// Basic sanity checks
	if (this->m_file == nullptr)
	{
		return EError::CannotRead;
	}

	// Nothing to read
	if (data.size() == 0)
	{
		return static_cast<std::size_t>(0);
	}

	// Clamp the requested read size to the file size reported in metadata to avoid extremely large spans
	// (this defends against corrupted metadata or misuse that results in huge span sizes and wasm OOB copies).
	const std::size_t fileSize = GetFileInfo().size;
	std::size_t toRead = data.size();
	toRead = std::min(toRead, fileSize);

	// If there's nothing to read after clamping, return 0
	if (toRead == 0)
	{
		return static_cast<std::size_t>(0);
	}

	// Perform the read using byte count (1) to avoid element-size confusion.
	const std::size_t read = fread(data.data(), 1, toRead, m_file);

	// If fread returned less than requested, check for error vs EOF.
	if (read < toRead)
	{
		if (ferror(m_file))
		{
			return EError::CannotRead;
		}
		// EOF reached — return number of bytes actually read (could be zero)
		return read;
	}

	// Successfully read requested bytes
	return read;
}

Hush::CFile::Result<std::span<std::byte>> Hush::CFile::Read(std::size_t size)
{
	(void)size;
	return EError::OperationNotSupported;
}

Hush::CFile::Result<void> Hush::CFile::Write(std::span<const std::byte> data)
{
	if (const auto written = fwrite(data.data(), sizeof(std::byte), data.size(), m_file); written != data.size())
	{
		return EError::CannotWrite;
	}

	return Success();
}

Hush::CFile::Result<void> Hush::CFile::Seek(std::size_t position)
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
