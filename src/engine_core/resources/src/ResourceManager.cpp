#include "ResourceManager.hpp"
#include "Assertions.hpp"
#include "Components/TextureComponent.hpp"
#include "IFile.hpp"
#include "IResourceManager.hpp"
#include "Logger.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "Shared/ImageTexture.hpp"
#include "VirtualFilesystem.hpp"
#include "crypto/Hashing.hpp"
#include <cstdint>
#include <cstring>
#include <magic_enum/magic_enum.hpp>
#include <string_view>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

void Hush::ResourceManager::Init(VirtualFilesystem *filesystem)
{
	this->m_filesystem = filesystem;
}

Hush::RefCounted *Hush::ResourceManager::IncreaseRefCount(const HandleId &handle)
{
	RefCounted &count = this->m_references[handle];
	count.count++;
	return &count;
}

Hush::RefCounted *Hush::ResourceManager::DecreaseRefCount(const HandleId &handle)
{
	RefCounted &count = this->m_references[handle];
	// TODO: Add condition to prevent overflow
	count.count--;
	if (count.count == 0)
	{
		this->m_deletionQueue.emplace_back(handle);
	}
	return &count;
}

const Hush::RefCounted &Hush::ResourceManager::GetRefCount(const Hush::HandleId &handle)
{
	return this->m_references[handle];
}

void Hush::ResourceManager::FreePending()
{
	// TODO: Parallel for
	for (const HandleId &handle : this->m_deletionQueue)
	{
		// Get the ref and call its deleter
		this->m_references.at(handle).deleter(reinterpret_cast<void *>(handle));
	}
	this->m_deletionQueue.clear();
}

Hush::Result<Hush::Ref<Hush::TextureComponent>, Hush::ResourceManager::EError> Hush::ResourceManager::LoadTexture(
	const std::string_view path, const TextureComponent::ECpuUnloadStrategy unloadStrategy)
{
	const uint64_t nameHash = Hashing::Fnv1a64(path);
	const auto &iterator = this->m_loadedResources.find(nameHash);
	if (iterator != this->m_loadedResources.end())
	{
		HandleId handle = this->m_loadedResources[nameHash];
		auto *texture = reinterpret_cast<TextureComponent *>(handle);
		return {this, texture};
	}

	// We don't have the texture loaded, so we need to load it
	auto file = m_filesystem->OpenFile(path, EFileOpenMode::Read);

	if (file.has_error())
	{
		switch (file.error())
		{
		case IFile::EError::FileDoesntExist:
		case IFile::EError::PathDoesntExist:
			return EError::FileNotFound;
		case IFile::EError::CannotRead:
			return EError::LoadFailed;
		default:
			Hush::LogFormat(ELogLevel::Error, "ResourceManager: Failed to load texture at {}", path);
			return EError::UnknownError;
		}
	}

	auto &filePtr = file.value();
	const auto fileInfo = filePtr->GetFileInfo();

	std::vector<std::byte> fileData(fileInfo.size);
	auto readResult = filePtr->Read(fileData);

	if (readResult.has_error())
	{
		Hush::LogFormat(ELogLevel::Error, "ResourceManager: Failed to read texture data at {}", path);
		return EError::LoadFailed;
	}

	// Now, we need to decode the image data and create a TextureComponent
	int width{};
	int height{};
	int channels{};
	// Force 4-channel (RGBA) decode so the pixel data always matches a
	// GPU-supported format.  WebGPU has no RGB8 texture format, so loading
	// as 3-channel would cause a format mismatch and validation errors.
	static constexpr int kDesiredChannels = 4;
	stbi_uc *imageData =
		stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(fileData.data()), static_cast<int>(fileData.size()),
							  &width, &height, &channels, kDesiredChannels);

	if (imageData == nullptr)
	{
		Hush::LogFormat(ELogLevel::Error, "ResourceManager: Failed to decode image data at {}", path);
		return EError::LoadFailed;
	}
	// We can now create the TextureComponent and store it in the loaded resources.
	// To do that, we need to create an Image and a Texture.
	//
	// Because we forced 4-channel decode above, the pixel data is always
	// RGBA regardless of the original file's channel count.
	const Graphics::ETextureFormat format = Graphics::ETextureFormat::RGBA8_UNORM;

	const auto depth = 1; // stbi_load only loads 2D images, so depth is always 1

	// Copy the decoded pixel data into a std::vector so the Image owns it.
	// stbi allocated imageData — we must free it after the copy.
	const size_t pixelDataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * kDesiredChannels;
	std::vector<std::byte> decodedPixels(pixelDataSize);
	std::memcpy(decodedPixels.data(), imageData, pixelDataSize);
	stbi_image_free(imageData);

	auto image = std::make_unique<Image>(std::move(decodedPixels), width, height, depth, format);

	// The IGraphicsTexture will be created by the ResourceUploadSystem when the TextureComponent is staged for upload,
	// so we can just create the TextureComponent with the image and unload strategy.
	auto *textureComponent = new TextureComponent(nullptr, std::move(image), unloadStrategy);

	const auto handle = reinterpret_cast<HandleId>(textureComponent);
	this->m_loadedResources[nameHash] = handle;

	return {this, textureComponent};
}

// Hush::Ref<Hush::ImageTexture> Hush::ResourceManager::LoadTexture(const std::string_view &name, const std::byte *data,
// 																 const size_t &size)
// {
// 	const uint64_t nameHash = Hashing::Fnv1a64(name);
// 	const auto &iterator = this->m_loadedResources.find(nameHash);
// 	if (iterator != this->m_loadedResources.end())
// 	{
// 		HandleId handle = this->m_loadedResources[nameHash];
// 		auto *texture = reinterpret_cast<ImageTexture *>(handle);
// 		return {this, texture};
// 	}
// 	auto *texture = new ImageTexture(data, size);
// 	return {this, texture};
// }

// Hush::Ref<Hush::ImageTexture> Hush::ResourceManager::LoadTexture(const std::string_view &path)
// {
// 	// Allocate the image texture and load it using the file system
// 	// Resolve the virtual path as an absolute path
// 	Result<std::string_view, VirtualFilesystem::EError> resolvedPath = this->m_filesystem->ResolveVirtualPath(path);
// 	HUSH_RESULT_ASSERT(resolvedPath, "Failed to load texture at {}", path);

// 	const uint64_t pathHash = Hashing::Fnv1a64(resolvedPath.value());
// 	const auto &iterator = this->m_loadedResources.find(pathHash);

// 	if (iterator != this->m_loadedResources.end())
// 	{
// 		HandleId handle = this->m_loadedResources[pathHash];
// 		auto *texture = reinterpret_cast<ImageTexture *>(handle);
// 		return {this, texture};
// 	}
// 	auto *texture = new ImageTexture(resolvedPath.value());
// 	const auto handle = reinterpret_cast<HandleId>(texture);
// 	this->m_loadedResources[pathHash] = handle;
// 	return {this, texture};
// }

Hush::Ref<Hush::Mesh> Hush::ResourceManager::LoadMesh(const std::string_view &path)
{
	(void)path;
	return {};
}
