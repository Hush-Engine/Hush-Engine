/*! \file TextureComponent.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-21
	\brief Texture ECS component.
*/

#pragma once

#include "RHI/IGraphicsTexture.hpp"
#include "RHI/Image.hpp"

#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <Hushgen.hpp>

#if __has_include("TextureComponent.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "TextureComponent.hushgen.hpp"
#endif

#include "HushBindings.hpp"

namespace Hush
{
	/// @brief ECS component intended to be used by entities.
	///
	/// This component owns the GPU and CPU resources for a texture.
	/// It is designed to be attached to entities that need to reference a texture resource, such as materials or
	/// renderable objects.
	struct [[hush::export, hush::reflect]] TextureComponent
	{
		HUSH_GENERATED_BODY

		friend class ResourceManager;

	public:
		enum class ECpuUnloadStrategy : uint8_t
		{
			/// @brief Unload the CPU image after uploading it to the GPU.
			/// This is the default strategy, as it minimizes memory usage by keeping only the GPU texture resident
			/// after upload. However, it means that the CPU image will not be available for readback or re-upload
			/// without reloading from disk.
			UnloadAfterUpload = 0,

			/// @brief Keep the CPU image resident in memory even after uploading to the GPU.
			/// This allows for readback or re-upload without needing to reload from disk, but increases memory usage by
			/// keeping both the CPU image and GPU texture resident.
			KeepResident = 1
		};

		TextureComponent() = default;

		explicit TextureComponent(std::unique_ptr<Graphics::IGraphicsTexture> texture, std::unique_ptr<Image> cpuImage,
								  ECpuUnloadStrategy cpuUnloadStrategy = ECpuUnloadStrategy::UnloadAfterUpload)
			: m_texture(std::move(texture)),
			  m_cpuImage(std::move(cpuImage)),
			  m_cpuUnloadStrategy(cpuUnloadStrategy)
		{
		}

		TextureComponent(const TextureComponent &) = delete;
		TextureComponent &operator=(const TextureComponent &) = delete;
		TextureComponent(TextureComponent &&) = default;
		TextureComponent &operator=(TextureComponent &&) = default;

		~TextureComponent() = default;

		[[nodiscard]]
		Graphics::IGraphicsTexture *GetTexture() const
		{
			return m_texture.get();
		}

		[[nodiscard]]
		uint32_t GetWidth() const
		{
			if (m_texture != nullptr)
			{
				return m_texture->GetWidth();
			}
			return 0;
		}

		[[nodiscard]]
		uint32_t GetHeight() const
		{
			if (m_texture != nullptr)
			{
				return m_texture->GetHeight();
			}
			return 0;
		}

		[[nodiscard]]
		uint32_t GetDepth() const
		{
			if (m_texture != nullptr)
			{
				return m_texture->GetDepth();
			}
			return 0;
		}

		[[nodiscard]]
		Graphics::ETextureFormat GetFormat() const
		{
			if (m_texture != nullptr)
			{
				return m_texture->GetFormat();
			}
			return Graphics::ETextureFormat::RGBA8_SRGB; // Default format if texture is null
		}

		[[nodiscard]]
		bool IsValid() const
		{
			return m_texture != nullptr;
		}

		[[nodiscard]]
		Graphics::IGraphicsTexture *GetGpuTexture() const
		{
			return m_texture.get();
		}

		[[nodiscard]]
		Image *GetCpuImage() const
		{
			return m_cpuImage.get();
		}

		[[nodiscard]]
		bool IsCpuImageValid() const
		{
			return m_cpuImage != nullptr;
		}

		/// @brief Set a new GPU texture for this component. This replaces the existing GPU texture, if any.
		///
		/// It is not encouraged to call this method directly from user code. Instead, the GPU texture should be set by
		/// the ResourceUploadSystem
		void SetGpuTexture(std::unique_ptr<Graphics::IGraphicsTexture> texture)
		{
			m_texture = std::move(texture);
		}

		[[nodiscard]]
		ECpuUnloadStrategy GetCpuUnloadStrategy() const
		{
			return m_cpuUnloadStrategy;
		}

	private:
		std::unique_ptr<Graphics::IGraphicsTexture> m_texture = nullptr;

		/// @brief Optional CPU-side image data for this texture, used for staging uploads or readback.
		/// Can be nullptr if no CPU copy is needed or available.
		std::unique_ptr<Image> m_cpuImage = nullptr;

		ECpuUnloadStrategy m_cpuUnloadStrategy = ECpuUnloadStrategy::UnloadAfterUpload;
	};

} // namespace Hush
