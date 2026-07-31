#include "ResourceManager.hpp"
#include "Assertions.hpp"
#include "Components/TextureComponent.hpp"
#include "HAsset.hpp"
#include "IFile.hpp"
#include "IResourceManager.hpp"
#include "Logger.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "Ref.hpp"
#include "Result.hpp"
#include "Shared/ImageTexture.hpp"
#include "VirtualFilesystem.hpp"
#include "crypto/Hashing.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <magic_enum/magic_enum.hpp>
#include <string_view>
#include <vector>
#include <zstd.h>

#include <stb/stb_image.h>

namespace
{
	/// Box-average downscale of tightly-packed RGBA8 pixels so the longest side is at most
	/// maxSize. Returns the (possibly unchanged) pixel buffer and writes the resulting
	/// dimensions to outW/outH. Non-RGBA8 / already-small inputs are returned unchanged.
	std::vector<std::byte> DownscaleRGBA8(const std::vector<std::byte> &src, int srcW, int srcH, uint32_t maxSize,
										  int &outW, int &outH)
	{
		const int longest = std::max(srcW, srcH);
		if (srcW <= 0 || srcH <= 0 || maxSize == 0 || longest <= static_cast<int>(maxSize) ||
			src.size() < static_cast<size_t>(srcW) * static_cast<size_t>(srcH) * 4U)
		{
			outW = srcW;
			outH = srcH;
			return src;
		}

		outW = std::max(1, srcW * static_cast<int>(maxSize) / longest);
		outH = std::max(1, srcH * static_cast<int>(maxSize) / longest);

		std::vector<std::byte> dst(static_cast<size_t>(outW) * static_cast<size_t>(outH) * 4U);

		for (int y = 0; y < outH; ++y)
		{
			const int sy0 = y * srcH / outH;
			const int sy1 = std::max(sy0 + 1, (y + 1) * srcH / outH);
			for (int x = 0; x < outW; ++x)
			{
				const int sx0 = x * srcW / outW;
				const int sx1 = std::max(sx0 + 1, (x + 1) * srcW / outW);

				uint32_t r = 0;
				uint32_t g = 0;
				uint32_t b = 0;
				uint32_t a = 0;
				uint32_t count = 0;
				for (int sy = sy0; sy < sy1; ++sy)
				{
					for (int sx = sx0; sx < sx1; ++sx)
					{
						const size_t si =
							((static_cast<size_t>(sy) * static_cast<size_t>(srcW)) + static_cast<size_t>(sx)) * 4U;
						r += std::to_integer<uint32_t>(src[si + 0]);
						g += std::to_integer<uint32_t>(src[si + 1]);
						b += std::to_integer<uint32_t>(src[si + 2]);
						a += std::to_integer<uint32_t>(src[si + 3]);
						++count;
					}
				}

				const size_t di = ((static_cast<size_t>(y) * static_cast<size_t>(outW)) + static_cast<size_t>(x)) * 4U;
				dst[di + 0] = static_cast<std::byte>(r / count);
				dst[di + 1] = static_cast<std::byte>(g / count);
				dst[di + 2] = static_cast<std::byte>(b / count);
				dst[di + 3] = static_cast<std::byte>(a / count);
			}
		}
		return dst;
	}
} // namespace

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
	const std::string_view path, const TextureComponent::ECpuUnloadStrategy unloadStrategy, uint32_t maxSize)
{
	uint64_t nameHash = Hashing::Fnv1a64(path);
	if (maxSize != 0)
	{
		// Cache a downscaled (e.g. thumbnail) load separately from the full-resolution
		// load of the same path so the two don't collide.
		nameHash ^= (static_cast<uint64_t>(maxSize) * 0x9E3779B97F4A7C15ULL);
	}
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

	std::vector<std::byte> fileData;
	fileData.resize(fileInfo.size);
	auto readResult = filePtr->Read(fileData);

	if (readResult.has_error())
	{
		Hush::LogFormat(ELogLevel::Error, "ResourceManager: Failed to read texture data at {}", path);
		return EError::LoadFailed;
	}

	// Check if this is a cooked HAsset blob
	int width{};
	int height{};
	int depth = 1;
	Graphics::ETextureFormat format = Graphics::ETextureFormat::RGBA8_UNORM;
	std::vector<std::byte> decodedPixels;

	if (fileData.size() >= sizeof(uint32_t))
	{
		uint32_t magic;
		std::memcpy(&magic, fileData.data(), sizeof(uint32_t));
		if (magic == HASSET_MAGIC)
		{
			// Cooked HAsset. Decompress and use directly
			auto asset = HAsset::Read(fileData);
			if (!asset.has_value())
			{
				Hush::LogFormat(ELogLevel::Error, "ResourceManager: Invalid HAsset at {}", path);
				return EError::InvalidData;
			}

			auto &header = asset->header;
			if (header.compression == ECompressionFormat::Zstd)
			{
				decodedPixels.resize(header.uncompressedSize);
				size_t result = ZSTD_decompress(decodedPixels.data(), header.uncompressedSize, asset->payload.data(),
												asset->payload.size());
				if (ZSTD_isError(result) != 0)
				{
					Hush::LogFormat(ELogLevel::Error, "ResourceManager: ZSTD decompress failed at {}: {}", path,
									ZSTD_getErrorName(result));
					return EError::LoadFailed;
				}
			}
			else
			{
				decodedPixels = std::move(asset->payload);
			}

			// Read texture extra for dimensions
			if (asset->extra.size() >= sizeof(HTextureExtra))
			{
				HTextureExtra texExtra;
				std::memcpy(&texExtra, asset->extra.data(), sizeof(HTextureExtra));
				width = static_cast<int>(texExtra.width);
				height = static_cast<int>(texExtra.height);
				depth = static_cast<int>(texExtra.depth);
			}
			else
			{
				// Fallback: infer from data size (RGBA8)
				const size_t px = decodedPixels.size() / 4;
				width = static_cast<int>(std::sqrt(px));
				height = width;
			}
		}
	}

	if (decodedPixels.empty())
	{
		// Not a cooked blob, decode with stb_image
		int channels{};
		static constexpr int kDesiredChannels = 4;
		stbi_uc *imageData =
			stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(fileData.data()), static_cast<int>(fileData.size()),
								  &width, &height, &channels, kDesiredChannels);

		if (imageData == nullptr)
		{
			Hush::LogFormat(ELogLevel::Error, "ResourceManager: Failed to decode image data at {}", path);
			return EError::LoadFailed;
		}

		const size_t pixelDataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * kDesiredChannels;
		decodedPixels.resize(pixelDataSize);
		std::memcpy(decodedPixels.data(), imageData, pixelDataSize);
		stbi_image_free(imageData);

		format = Graphics::ETextureFormat::RGBA8_UNORM;
		depth = 1;
	}

	// Optionally box-downscale the decoded RGBA8 image (e.g. for thumbnails) so we upload a
	// small GPU texture instead of the full-resolution one.
	if (maxSize != 0 && depth == 1)
	{
		int newWidth = width;
		int newHeight = height;
		decodedPixels = DownscaleRGBA8(decodedPixels, width, height, maxSize, newWidth, newHeight);
		width = newWidth;
		height = newHeight;
	}

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
