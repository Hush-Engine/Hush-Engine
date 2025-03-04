/*! \file CFile.hpp
	\author Alan Ramirez
	\date 2024-12-22
	\brief C File implementation
*/

#pragma once
#include "IFile.hpp"

#include <cstdio>

namespace Hush
{
	class CFile final : public IFile
	{
	public:
		CFile(FILE *file, FileMetadata &&fileMetadata)
			: m_file(file),
			  m_metadata(std::move(fileMetadata))
		{
		}

		~CFile() override;

		/// @copydoc IFile::Read(std::span<std::byte>)
		[[nodiscard]]
		Result<std::size_t> Read(std::span<std::byte> data) override;

		/// @copydoc IFile::Read(std::size_t)
		[[nodiscard]]
		Result<std::span<std::byte>> Read(std::size_t size) override;

		/// @copydoc IFile::Write(std::span<const std::byte>)
		[[nodiscard]]
		Result<void> Write(std::span<const std::byte> data) override;

		/// @copydoc IFile::Seek(std::size_t)
		[[nodiscard]]
		Result<void> Seek(std::size_t position) override;

		void Close() override;

		/// @copydoc IFile::GetMetadata
		[[nodiscard]]
		const FileMetadata &GetMetadata() const override
		{
			return m_metadata;
		}

	private:
		FILE *m_file;
		FileMetadata m_metadata;
	};

}; // namespace Hush