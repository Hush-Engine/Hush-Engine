/*! \file VirtualFilesystem.cpp
	\author Alan Ramirez
	\date 2024-12-07
	\brief A Virtual filesystem implementation
*/

#include "VirtualFilesystem.hpp"
#include "FileSystem.hpp"

#include <Logger.hpp>
#include <filesystem>
#include <memory>
#include <optional>
#include <ranges>
#include <string_view>
#include "StringUtils.hpp"
#include "IFile.hpp"

Hush::VirtualFilesystem::VirtualFilesystem() = default;

Hush::VirtualFilesystem::~VirtualFilesystem() = default;

Hush::VirtualFilesystem::VirtualFilesystem(VirtualFilesystem &&rhs) noexcept
	: m_mountedFileSystems(std::move(rhs.m_mountedFileSystems))
{
}

Hush::VirtualFilesystem &Hush::VirtualFilesystem::operator=(VirtualFilesystem &&rhs) noexcept
{
	if (this != &rhs)
	{
		m_mountedFileSystems = std::move(rhs.m_mountedFileSystems);
	}

	return *this;
}
void Hush::VirtualFilesystem::Unmount(std::string_view virtualPath)
{
	m_mountedFileSystems.erase(
		std::ranges::remove_if(m_mountedFileSystems,
							   [&](const MountPoint &mountPoint) { return mountPoint.path == virtualPath; })
			.begin(),
		m_mountedFileSystems.end());
}

std::vector<Hush::FileInfo> Hush::VirtualFilesystem::ListPath(std::string_view virtualPath, EListOptions options)
{
	std::optional<ResolvedPath> resolved = this->ResolveFileSystem(virtualPath);
	
	if (!resolved)
	{
		LogFormat(ELogLevel::Debug, "Mount point for {} not found", virtualPath);
		return {};
	}
	
	(void)options;
	auto result = resolved->filesystem->ListPath(resolved->path);
	if (result.has_error()) {
		return {};
	}
	return result.value();
}

Hush::Result<std::unique_ptr<Hush::IFile>, Hush::IFile::EError> Hush::VirtualFilesystem::OpenFile(
	std::string_view virtualPath, EFileOpenMode mode)
{
	auto resolved = ResolveFileSystem(virtualPath);

	if (!resolved)
	{
		LogFormat(ELogLevel::Debug, "Mount point for {} not found", virtualPath);
		return IFile::EError::FileDoesntExist;
	}

	// Open the file
	auto &filesystem = resolved->filesystem;
	auto &path = resolved->path;

	auto file = filesystem->OpenFile(virtualPath, path, mode);

	return file;
}

void Hush::VirtualFilesystem::MountFileSystemInternal(std::string_view path,
													  std::unique_ptr<IFileSystem> resourceLoader)
{
	m_mountedFileSystems.emplace_back(std::string(path), std::move(resourceLoader));
}


Hush::Result<std::string_view, Hush::VirtualFilesystem::EError> Hush::VirtualFilesystem::ResolveVirtualPath(const std::string_view& path) {
	std::optional<ResolvedPath> resolvedPath = this->ResolveFileSystem(path);
	if (!resolvedPath) {
		LogFormat(ELogLevel::Debug, "Mount point for {} not found", path);
		return EError::FileDoesntExist;
	}
	return resolvedPath->path;
}

std::optional<Hush::VirtualFilesystem::ResolvedPath> Hush::VirtualFilesystem::ResolveFileSystem(std::string_view path)
{
	if (std::filesystem::path(path).is_absolute()) {
		return ResolvedPath {
			.filesystem = this->m_mountedFileSystems[0].filesystem.get(),
			.path = path
		};
	}

	// We need to iterate on all filesystems in backward order.
	for (auto start = this->m_mountedFileSystems.begin(); start != this->m_mountedFileSystems.end(); ++start)
	{
		std::string &mountPoint = start->path;
		std::unique_ptr<IFileSystem> &filesystem = start->filesystem;

		if (path.starts_with(mountPoint))
		{
			std::string_view relativePath = path.substr(mountPoint.size());

			return ResolvedPath{
				.filesystem = filesystem.get(),
				.path = relativePath,
			};
		}
	}

	return {};
}


Hush::Result<Hush::FileInfo, Hush::IFile::EError> Hush::VirtualFilesystem::GetFirstMatchingSubstr(const std::filesystem::path& parent, const std::string_view& stemSubstr) {
	if (!std::filesystem::exists(parent)) {
		return IFile::EError::PathDoesntExist;
	}

	// Get the parent path's iterator
	for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(parent)) {
		std::string stem = entry.path().stem().string();
		if (stem.find(stemSubstr) == std::string::npos) {
			continue;
		}
		std::string extension = entry.path().extension().string();
		std::string rawExtension = StringUtils::ToUpper(StringUtils::SubstrView(extension, 1, static_cast<int32_t>(extension.size())));
		
		return FileInfo {
			.path = entry.path(),
			.size = entry.file_size(),
			.flags = EFileFlags::File,
			.extension = IFileSystem::ToKnownExtension(rawExtension)
		};
	}
	
	return IFile::EError::FileDoesntExist;
}


