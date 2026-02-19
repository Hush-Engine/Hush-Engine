/*! \file GraphicsTypes.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Common types, enumerations, and structures for graphics abstraction
*/

#include "IGraphicsDevice.hpp"
#include "GraphicsResources.hpp"

namespace Hush::Graphics
{
	void TextureResource::CreateResource(const TextureDescriptor &textureDescriptor, IGraphicsDevice *device)
	{
		this->descriptor = textureDescriptor;
		this->texture = device->CreateTexture(descriptor);
	}

	void TextureResource::DestroyResource([[maybe_unused]] const TextureDescriptor &textureDescriptor,
										  [[maybe_unused]] IGraphicsDevice *device)
	{
		this->texture.reset();
	}
} // namespace Hush::Graphics
