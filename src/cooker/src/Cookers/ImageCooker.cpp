#include "ImageCooker.hpp"
#include "HAsset.hpp"
#include "crypto/Hashing.hpp"
#include "Logger.hpp"
#include <cstring>
#include <vector>

#include <stb/stb_image.h>

// Optional: mipmap generation
#if __has_include(<stb/stb_image_resize2.h>)
#define HUSH_HAS_STB_RESIZE2
#include <stb/stb_image_resize2.h>
#endif

// Optional: BCn block compression (vendored bc7enc)
#if __has_include(<bc7enc/bc7enc.h>)
#define HUSH_HAS_BC7ENC
#include <bc7enc/bc7enc.h>
#include <bc7enc/rgbcx.h>
#endif

namespace Hush
{

	std::span<const EFileExtension> ImageCooker::SupportedExtensions() const
	{
		return EXTENSIONS;
	}

	bool ImageCooker::CanCook(const FileInfo &info) const
	{
		return info.extension == EFileExtension::PNG || info.extension == EFileExtension::JPEG;
	}

	HMeta ImageCooker::DefaultMeta(EFileExtension ext, std::string_view srcVPath) const
	{
		HMeta meta;
		meta.id = Hashing::Fnv1a(srcVPath);
		meta.assetType = "texture";
		meta.outputFormat = EAssetFormat::RGBA8_UNORM;
		meta.compression = ECompressionFormat::Zstd;
		meta.texture.sRGB = true;
		meta.texture.generateMipmaps = false;
		(void)ext;
		return meta;
	}

	/// Generate mip chain using stb_image_resize2 (if available).
	/// Otherwise, returns the single full-res mip (mipCount = 1).
	static void GenerateMips(const std::vector<std::byte> &srcRGBA, int w, int h, std::vector<std::byte> &outMipData,
							 uint32_t &outMipCount)
	{
#if defined(HUSH_HAS_STB_RESIZE2)
		int mipW = w;
		int mipH = h;
		outMipData = srcRGBA; // top mip
		outMipCount = 1;

		while (mipW > 1 || mipH > 1)
		{
			int nextW = std::max(1, mipW / 2);
			int nextH = std::max(1, mipH / 2);
			size_t prevSize = static_cast<size_t>(mipW) * static_cast<size_t>(mipH) * 4;
			const std::byte *prevData = outMipData.data() + outMipData.size() - prevSize;

			size_t nextSize = static_cast<size_t>(nextW) * static_cast<size_t>(nextH) * 4;
			size_t oldSize = outMipData.size();
			outMipData.resize(oldSize + nextSize);

			stbir_resize_uint8_srgb(reinterpret_cast<const unsigned char *>(prevData), mipW, mipH, 0,
									reinterpret_cast<unsigned char *>(outMipData.data() + oldSize), nextW, nextH, 0,
									STBIR_RGBA, STBIR_ALPHA_CHANNEL_NONE, STBIR_FLAG_ALPHA_PREMULTIPLIED);

			mipW = nextW;
			mipH = nextH;
			outMipCount++;
		}
#else
		// No resize library available — keep single mip
		outMipData = srcRGBA;
		outMipCount = 1;
		(void)w;
		(void)h;
#endif
	}

	/// Encode RGBA8 pixels to BCn format (if bc7enc is available).
	/// Returns the input data unchanged if bc7enc is not available.
	static std::vector<std::byte> EncodeBCn(std::span<const std::byte> rgba, int w, int h,
											const std::string &gpuCompression)
	{
#if defined(HUSH_HAS_BC7ENC)
		(void)w;
		(void)h;

		// Initialize bc7enc once
		static bool initialized = false;
		if (!initialized)
		{
			bc7enc_init();
			rgbcx::init(bc7enc_compress_mode::BC7ENC_COMPRESS_MODE_DEFAULT);
			initialized = true;
		}

		// Map compression string to format
		enum class BCNFormat
		{
			BC1,
			BC3,
			BC5,
			BC7
		};
		BCNFormat fmt = BCNFormat::BC7;
		if (gpuCompression == "BC1")
			fmt = BCNFormat::BC1;
		else if (gpuCompression == "BC3")
			fmt = BCNFormat::BC3;
		else if (gpuCompression == "BC5")
			fmt = BCNFormat::BC5;
		else if (gpuCompression == "BC7")
			fmt = BCNFormat::BC7;
		else
			return std::vector<std::byte>(rgba.begin(), rgba.end()); // unsupported, pass through

		// Determine block size
		const size_t numBlocks = (static_cast<size_t>(w + 3) / 4) * (static_cast<size_t>(h + 3) / 4);
		size_t blockSize = 16; // BC7 = 16 bytes/block
		if (fmt == BCNFormat::BC1)
			blockSize = 8;

		std::vector<std::byte> result(numBlocks * blockSize);
		const uint8_t *src = reinterpret_cast<const uint8_t *>(rgba.data());
		uint8_t *dst = reinterpret_cast<uint8_t *>(result.data());

		for (size_t by = 0; by < numBlocks; ++by)
		{
			uint8_t pixels[4 * 4 * 4]; // 4x4 block of RGBA8
			// Gather 4x4 block from source
			// (simplified: just copy the first 16 pixels for each block — proper implementation
			//  would iterate over all blocks with correct offsets)
			size_t bx = by % ((w + 3) / 4);
			size_t byIdx = by / ((w + 3) / 4);

			for (int py = 0; py < 4; ++py)
			{
				for (int px = 0; px < 4; ++px)
				{
					int sx = static_cast<int>(bx * 4 + px);
					int sy = static_cast<int>(byIdx * 4 + py);
					if (sx >= w || sy >= h)
					{
						// Edge pixel: clamp
						sx = std::min(sx, w - 1);
						sy = std::min(sy, h - 1);
					}
					size_t srcIdx = static_cast<size_t>(sy) * static_cast<size_t>(w) * 4 + static_cast<size_t>(sx) * 4;
					size_t dstIdx = static_cast<size_t>(py * 4 + px) * 4;
					std::memcpy(pixels + dstIdx, src + srcIdx, 4);
				}
			}

			switch (fmt)
			{
			case BCNFormat::BC1: {
				rgbcx::encode_bc1(16, dst, pixels, 0, 0);
				dst += 8;
				break;
			}
			case BCNFormat::BC3: {
				rgbcx::encode_bc3(16, dst, pixels);
				dst += 16;
				break;
			}
			case BCNFormat::BC5: {
				rgbcx::encode_bc5(16, dst, pixels, 0);
				dst += 16;
				break;
			}
			case BCNFormat::BC7: {
				bc7enc_compress_block_params params{};
				bc7enc_compress_block_params_init(&params);
				bc7enc_compress_block(dst, pixels, &params);
				dst += 16;
				break;
			}
			}
		}

		return result;
#else
		(void)w;
		(void)h;
		(void)gpuCompression;
		return std::vector<std::byte>(rgba.begin(), rgba.end());
#endif
	}

	Result<ImageCooker::CookResult, ECookError> ImageCooker::Cook(std::span<const std::byte> input, const HMeta &meta,
																  const CookContext &ctx)
	{
		int width{};
		int height{};
		int channels{};

		static constexpr int kDesiredChannels = 4;

		stbi_uc *imageData =
			stbi_load_from_memory(reinterpret_cast<const stbi_uc *>(input.data()), static_cast<int>(input.size()),
								  &width, &height, &channels, kDesiredChannels);

		if (imageData == nullptr)
		{
			LogFormat(ELogLevel::Error, "ImageCooker: failed to decode {}", ctx.sourceVPath);
			return ECookError::DecodeFailed;
		}

		const size_t pixelDataSize = static_cast<size_t>(width) * static_cast<size_t>(height) * kDesiredChannels;
		std::vector<std::byte> pixels(pixelDataSize);
		std::memcpy(pixels.data(), imageData, pixelDataSize);
		stbi_image_free(imageData);

		// Generate mip chain (if requested and available)
		std::vector<std::byte> mipData;
		uint32_t mipCount = 1;
		if (meta.texture.generateMipmaps)
		{
			GenerateMips(pixels, width, height, mipData, mipCount);
		}
		else
		{
			mipData = std::move(pixels);
		}

		// Apply BCn compression (if requested and available)
		std::vector<std::byte> finalData;
		EAssetFormat finalFormat = meta.outputFormat;

		if (meta.texture.gpuCompression != "none")
		{
			finalData = EncodeBCn(mipData, width, height, meta.texture.gpuCompression);

			// Map compression string to format enum
			if (meta.texture.gpuCompression == "BC1")
				finalFormat = EAssetFormat::DXT1;
			else if (meta.texture.gpuCompression == "BC3")
				finalFormat = EAssetFormat::DXT5;
			else if (meta.texture.gpuCompression == "BC5")
				finalFormat = EAssetFormat::BC5;
			else if (meta.texture.gpuCompression == "BC7")
				finalFormat = EAssetFormat::BC7;
		}
		else
		{
			finalData = std::move(mipData);
		}

		// Build HTextureExtra
		HTextureExtra texExtra{};
		texExtra.width = static_cast<uint32_t>(width);
		texExtra.height = static_cast<uint32_t>(height);
		texExtra.depth = 1;
		texExtra.mipCount = mipCount;

		switch (finalFormat)
		{
		case EAssetFormat::RGBA8_UNORM:
			texExtra.gpuFormat = 1;
			break;
		case EAssetFormat::BGRA8_UNORM:
			texExtra.gpuFormat = 2;
			break;
		case EAssetFormat::DXT1:
			texExtra.gpuFormat = 3;
			break;
		case EAssetFormat::DXT5:
			texExtra.gpuFormat = 4;
			break;
		case EAssetFormat::BC7:
			texExtra.gpuFormat = 5;
			break;
		default:
			texExtra.gpuFormat = 1;
			break;
		}

		std::vector<std::byte> extra(sizeof(HTextureExtra));
		std::memcpy(extra.data(), &texExtra, sizeof(HTextureExtra));

		CookResult result;
		result.payload = std::move(finalData);
		result.extra = std::move(extra);
		result.format = finalFormat;
		return result;
	}

} // namespace Hush
