#include "PakFileSystem.hpp"
#include "Result.hpp"
#include "crypto/Hashing.hpp"
#include "Logger.hpp"
#include <algorithm>
#include <cstring>
#include <fstream>

#if defined(HUSH_PLATFORM_EMSCRIPTEN)
// Emscripten: full-read fallback (no mmap)
#elif defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#else
// Linux / macOS
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace Hush
{

// ── MappedFile ────────────────────────────────────────────────────────────

MappedFile::MappedFile(MappedFile &&other) noexcept
	: m_data(other.m_data),
	  m_size(other.m_size),
	  m_osHandle(other.m_osHandle),
	  m_osMapping(other.m_osMapping),
	  m_fallback(std::move(other.m_fallback))
{
	other.m_data = nullptr;
	other.m_size = 0;
	other.m_osHandle = nullptr;
	other.m_osMapping = nullptr;
}

MappedFile &MappedFile::operator=(MappedFile &&other) noexcept
{
	if (this != &other)
	{
		Close();
		m_data = other.m_data;
		m_size = other.m_size;
		m_osHandle = other.m_osHandle;
		m_osMapping = other.m_osMapping;
		m_fallback = std::move(other.m_fallback);
		other.m_data = nullptr;
		other.m_size = 0;
		other.m_osHandle = nullptr;
		other.m_osMapping = nullptr;
	}
	return *this;
}

MappedFile::~MappedFile()
{
	Close();
}

bool MappedFile::Open(std::string_view path)
{
#if defined(HUSH_PLATFORM_EMSCRIPTEN)
	// Emscripten: full-read into memory
	std::ifstream file(std::string(path), std::ios::binary | std::ios::ate);
	if (!file)
	{
		return false;
	}

	m_size = static_cast<size_t>(file.tellg());
	file.seekg(0);
	m_fallback.resize(m_size);
	file.read(reinterpret_cast<char *>(m_fallback.data()), static_cast<std::streamsize>(m_size));
	m_data = m_fallback.data();
	return true;

#elif defined(_WIN32)
	// Windows: CreateFileMapping + MapViewOfFile
	std::wstring wpath(path.begin(), path.end());
	HANDLE hFile = CreateFileW(wpath.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
							   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	LARGE_INTEGER fileSize;
	GetFileSizeEx(hFile, &fileSize);
	m_size = static_cast<size_t>(fileSize.QuadPart);

	HANDLE hMapping = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);
	if (hMapping == nullptr)
	{
		CloseHandle(hFile);
		return false;
	}

	void *ptr = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
	if (ptr == nullptr)
	{
		CloseHandle(hMapping);
		CloseHandle(hFile);
		return false;
	}

	m_osHandle = hFile;
	m_osMapping = hMapping;
	m_data = static_cast<const std::byte *>(ptr);
	return true;

#else
	// Linux / macOS: mmap
	int fd = open(std::string(path).c_str(), O_RDONLY);
	if (fd < 0)
	{
		return false;
	}

	struct stat st;
	if (fstat(fd, &st) < 0)
	{
		close(fd);
		return false;
	}
	m_size = static_cast<size_t>(st.st_size);

	void *ptr = mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, fd, 0);
	if (ptr == MAP_FAILED)
	{
		close(fd);
		return false;
	}

	close(fd); // file descriptor no longer needed after mmap

	m_osHandle = ptr; // store mmap pointer for cleanup
	m_data = static_cast<const std::byte *>(ptr);
	return true;
#endif
}

void MappedFile::Close()
{
#if defined(HUSH_PLATFORM_EMSCRIPTEN)
	m_fallback.clear();
#elif defined(_WIN32)
	if (m_data)
	{
		UnmapViewOfFile(const_cast<std::byte *>(m_data));
	}
	if (m_osMapping)
	{
		CloseHandle(static_cast<HANDLE>(m_osMapping));
	}
	if (m_osHandle)
	{
		CloseHandle(static_cast<HANDLE>(m_osHandle));
	}
#else
	if (m_data)
	{
		munmap(const_cast<std::byte *>(m_data), m_size);
	}
#endif
	m_data = nullptr;
	m_size = 0;
	m_osHandle = nullptr;
	m_osMapping = nullptr;
}

// ── PakFileSystem ────────────────────────────────────────────────────────

PakFileSystem::PakFileSystem(std::string_view bundlePath)
{
	if (!m_mappedFile.Open(bundlePath))
	{
		LogFormat(ELogLevel::Error, "PakFileSystem: failed to open bundle: {}", bundlePath);
		return;
	}

	auto data = std::span<const std::byte>(m_mappedFile.Data(), m_mappedFile.Size());
	auto pak = HushPak::Read(data);
	if (!pak.has_value())
	{
		LogFormat(ELogLevel::Error, "PakFileSystem: invalid bundle: {}", bundlePath);
		m_mappedFile = MappedFile(); // reset
		return;
	}

	m_pak = std::move(pak.value());
}

Result<std::unique_ptr<IFile>, IFile::EError>
PakFileSystem::OpenFile(std::filesystem::path /*vfsPath*/, std::filesystem::path path, EFileOpenMode mode)
{

	if (mode != EFileOpenMode::Read)
	{
		return IFile::EError::OperationNotSupported;
	}

	// Normalize the path: use forward slashes, strip leading slash
	std::string normalized = path.string();
	std::replace(normalized.begin(), normalized.end(), '\\', '/');
	if (!normalized.empty() && normalized[0] == '/')
	{
		normalized = normalized.substr(1);
	}

	const uint64_t nameHash = Hashing::Fnv1a64(normalized);
	const PakEntry *entry = m_pak.FindEntry(nameHash);
	if (entry == nullptr)
	{
		return IFile::EError::FileDoesntExist;
	}

	auto bundleSpan = std::span<const std::byte>(m_mappedFile.Data(), m_mappedFile.Size());
	std::span<const std::byte> entryData = m_pak.EntryData(*entry, bundleSpan);
	if (entryData.empty())
	{
		return IFile::EError::CannotRead;
	}

	FileInfo info;
	info.path = normalized;
	info.size = entryData.size();
	info.mode = EFileOpenMode::Read;
	info.flags = EFileFlags::File;

	return std::make_unique<PakFile>(std::move(normalized), entryData, info);
}

Result<std::vector<FileInfo>, IFile::EError>
PakFileSystem::ListPath(const std::string_view &path)
{
	std::string prefix(path);

	std::replace(prefix.begin(), prefix.end(), '\\', '/');
	if (!prefix.empty() && prefix.back() != '/')
	{
		prefix += '/';
	}

	std::vector<FileInfo> results;

	for (const auto &entry : m_pak.directory)
	{
		if (entry.nameOffset + entry.nameLength > m_pak.stringTable.size())
		{
			continue;
		}
		std::string_view entryName(m_pak.stringTable.data() + entry.nameOffset, entry.nameLength);

		if (entryName.find(prefix) != 0)
		{
			continue;
		}

		FileInfo info;
		info.path = entryName;
		info.size = static_cast<size_t>(entry.dataSize);
		info.mode = EFileOpenMode::Read;
		info.flags = EFileFlags::File;
		results.push_back(info);
	}

	return results;
}

// ── PakFile ──────────────────────────────────────────────────────────────

PakFile::PakFile(std::string /*virtualPath*/, std::span<const std::byte> data, const FileInfo &info)
	: m_info(info),
	  m_data(data)
{
}

const FileInfo &PakFile::GetFileInfo() const
{
	return m_info;
}

Result<void, IFile::EError> PakFile::Write(std::span<const std::byte> /*data*/)
{
	return IFile::EError::OperationNotSupported;
}

Result<std::size_t, IFile::EError> PakFile::Read(std::span<std::byte> data)
{
	const size_t toRead = std::min(data.size(), m_data.size() - m_position);
	std::memcpy(data.data(), m_data.data() + m_position, toRead);
	m_position += toRead;
	return toRead;
}

Result<std::span<std::byte>, IFile::EError> PakFile::Read(std::size_t size)
{
	if (m_position + size > m_data.size())
	{
		size = m_data.size() - m_position;
	}
	auto *ptr = const_cast<std::byte *>(m_data.data() + m_position);
	m_position += size;
	return std::span<std::byte>(ptr, size);
}

Result<void, IFile::EError> PakFile::Seek(std::size_t position)
{
	if (position > m_data.size())
	{
		return IFile::EError::CannotRead;
	}
	m_position = position;
	return Success();
}

void PakFile::Close()
{
	// No-op; data is owned by PakFileSystem
}

} // namespace Hush
