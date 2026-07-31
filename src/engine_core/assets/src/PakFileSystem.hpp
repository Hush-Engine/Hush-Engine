#pragma once

#include "HushPak.hpp"
#include "FileSystem.hpp"
#include "IFile.hpp"
#include <memory>
#include <vector>
#include <string>

namespace Hush
{

	/// Cross-platform memory-mapped file (or full-read fallback for Emscripten).
	struct MappedFile
	{
		MappedFile() = default;
		MappedFile(const MappedFile &) = delete;
		MappedFile &operator=(const MappedFile &) = delete;
		MappedFile(MappedFile &&other) noexcept;
		MappedFile &operator=(MappedFile &&other) noexcept;
		~MappedFile();

		/// Map the file at the given path into memory.
		/// On Emscripten, falls back to reading the entire file into a buffer.
		[[nodiscard]]
		bool Open(std::string_view path);

		/// Pointer to the mapped data (read-only).
		const std::byte *Data() const
		{
			return m_data;
		}

		/// Size of the mapped data.
		size_t Size() const
		{
			return m_size;
		}

	private:
		void Close();

		const std::byte *m_data = nullptr;
		size_t m_size = 0;

		// OS-specific handles for cleanup
		void *m_osHandle = nullptr;	 // Windows: file handle (HANDLE)
		void *m_osMapping = nullptr; // Windows: mapping handle (HANDLE)

		std::vector<std::byte> m_fallback; // Emscripten fallback buffer
	};

	/// Read-only IFileSystem backed by a .hushpak bundle.
	/// Uses memory-mapped I/O on supported platforms, full-read fallback on Emscripten.
	/// Supports O(log n) lookup via binary search on Fnv1a64 name hashes.
	class PakFileSystem final : public IFileSystem
	{
	public:
		/// Load a .hushpak file from the given OS path.
		explicit PakFileSystem(std::string_view bundlePath);

		PakFileSystem(const PakFileSystem &) = delete;
		PakFileSystem(PakFileSystem &&) noexcept = default;
		PakFileSystem &operator=(const PakFileSystem &) = delete;
		PakFileSystem &operator=(PakFileSystem &&) noexcept = default;

		~PakFileSystem() override = default;

		Result<std::unique_ptr<IFile>, IFile::EError> OpenFile(std::filesystem::path vfsPath,
															   std::filesystem::path path, EFileOpenMode mode) override;

		Result<std::vector<FileInfo>, IFile::EError> ListPath(const std::string_view &path) override;

	private:
		MappedFile m_mappedFile;
		HushPak m_pak;
	};

	/// An IFile implementation that reads from a span into a loaded .hushpak bundle.
	class PakFile final : public IFile
	{
	public:
		PakFile(std::string virtualPath, std::span<const std::byte> data, const FileInfo &info);

		PakFile(const PakFile &) = delete;
		PakFile(PakFile &&) noexcept = default;
		PakFile &operator=(const PakFile &) = delete;
		PakFile &operator=(PakFile &&) noexcept = default;

		~PakFile() override = default;

		const FileInfo &GetFileInfo() const override;
		Result<void> Write(std::span<const std::byte> data) override;
		Result<std::size_t> Read(std::span<std::byte> data) override;
		Result<std::span<std::byte>> Read(std::size_t size) override;
		Result<void> Seek(std::size_t position) override;
		void Close() override;

	private:
		FileInfo m_info;
		std::span<const std::byte> m_data;
		size_t m_position = 0;
	};

} // namespace Hush
