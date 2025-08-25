#include "ResourceManager.hpp"
#include "Assertions.hpp"
#include "IResourceManager.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "Shared/ImageTexture.hpp"
#include "VirtualFilesystem.hpp"
#include "crypto/Hashing.hpp"
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include <string_view>


void Hush::ResourceManager::Init(VirtualFilesystem* filesystem) {
	this->m_filesystem = filesystem;
}

Hush::RefCounted* Hush::ResourceManager::IncreaseRefCount(const HandleId& handle) {
	RefCounted& count = this->m_references[handle];
	count.count++;
	return &count;
}

Hush::RefCounted* Hush::ResourceManager::DecreaseRefCount(const HandleId& handle) {
	RefCounted& count = this->m_references[handle];
	// TODO: Add condition to prevent overflow
	count.count--;
	if (count.count == 0) {
		this->m_deletionQueue.emplace_back(handle);
	}
	return &count;
}

const Hush::RefCounted& Hush::ResourceManager::GetRefCount(const Hush::HandleId& handle) {
	return this->m_references[handle];
}


void Hush::ResourceManager::FreePending() {
	// TODO: Parallel for
	for(const HandleId& handle : this->m_deletionQueue) {
		// Get the ref and call its deleter
		this->m_references[handle].deleter(reinterpret_cast<void*>(handle));
	}
}

Hush::Ref<Hush::ImageTexture> Hush::ResourceManager::LoadTexture(const std::string_view& name, const std::byte* data, const size_t& size) {
	const uint64_t nameHash = Hashing::Fnv1a64(name);
	const auto& iterator = this->m_loadedResources.find(nameHash);
	if (iterator != this->m_loadedResources.end()) {
		HandleId handle = this->m_loadedResources[nameHash];
		auto* texture = reinterpret_cast<ImageTexture*>(handle);
		return {this, texture};
	}
	auto* texture = new ImageTexture(data, size);
	return {this, texture};
}

Hush::Ref<Hush::ImageTexture> Hush::ResourceManager::LoadTexture(const std::string_view& path) {
	// Allocate the image texture and load it using the file system
	// Resolve the virtual path as an absolute path
	Result<std::string_view, VirtualFilesystem::EError> resolvedPath = this->m_filesystem->ResolveVirtualPath(path);
	HUSH_RESULT_ASSERT(resolvedPath, "Failed to load texture at {}", path);
	
	const uint64_t pathHash = Hashing::Fnv1a64(resolvedPath.value());
	const auto& iterator = this->m_loadedResources.find(pathHash);
	
	if (iterator != this->m_loadedResources.end()) {
		HandleId handle = this->m_loadedResources[pathHash];
		auto* texture = reinterpret_cast<ImageTexture*>(handle);
		return {this, texture};
	}
	auto* texture = new ImageTexture(resolvedPath.value());
	const auto handle = reinterpret_cast<HandleId>(texture);
	this->m_loadedResources[pathHash] = handle;
	return {this, texture};
}

Hush::Ref<Hush::Mesh> Hush::ResourceManager::LoadMesh(const std::string_view& path) {
	(void)path;
	return {this, nullptr};
}

