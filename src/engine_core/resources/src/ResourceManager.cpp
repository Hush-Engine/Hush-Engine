#include "ResourceManager.hpp"
#include "Assertions.hpp"
#include "IResourceManager.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "Shared/ImageTexture.hpp"
#include "VirtualFilesystem.hpp"
#include "crypto/Hashing.hpp"
#include "filesystem/CFileSystem/CFileSystem.hpp"
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <string_view>

Hush::ResourceManager::ResourceManager() {
	this->m_filesystem.MountFileSystem<CFileSystem>("res://", "./");
}

Hush::RefCounted* Hush::ResourceManager::IncreaseRefCount(const HandleId& handle) {
	RefCounted& count = this->m_references[handle];
	count.count++;
	return &count;
}

void Hush::ResourceManager::DecreaseRefCount(const HandleId& handle) {
	RefCounted& count = this->m_references[handle];
	// TODO: Add condition to prevent overflow
	count.count--;
	if (count.count == 0) {
		this->m_deletionQueue.emplace_back(handle);
	}
}


const Hush::RefCounted& Hush::ResourceManager::GetRefCount(const Hush::HandleId& handle) {
	return this->m_references[handle];
}


Hush::Ref<Hush::ImageTexture> Hush::ResourceManager::LoadTexture(const std::string_view& path) {
	// Allocate the image texture and load it using the file system
	// Resolve the virtual path as an absolute path
	Result<std::string_view, VirtualFilesystem::EError> resolvedPath = this->m_filesystem.ResolveVirtualPath(path);
	HUSH_RESULT_ASSERT(resolvedPath, "Failed to load texture at {}", path);
	
	const uint64_t pathHash = Hashing::Fnv1a64(path);
	const auto& iterator = this->m_loadedResources.find(pathHash);
	
	if (iterator != this->m_loadedResources.end()) {
		HandleId handle = this->m_loadedResources[pathHash];
		auto* texture = reinterpret_cast<ImageTexture*>(handle);
		return {this, texture};
	}
	auto* texture = new ImageTexture(path);
	return {this, texture};
}

