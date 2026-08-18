#pragma once

namespace Hush
{
	class ResourceManager;
	class MeshReference;
	class Scene;
	class VirtualFilesystem;

	namespace Graphics
	{
		class IGraphicsTexture;
		class IGraphicsDevice;
		class Material3D;
		struct Material3DDescriptor;
	} // namespace Graphics

	// TODO: Move to its own file
	struct RenderingContext
	{
		const Graphics::Material3DDescriptor *materialDescriptor;
		Scene *activeScene; // Optional for asset cooking pipeline, required for direct loads where we instance comps
		ResourceManager *resourceManager;
		VirtualFilesystem *virtualFilesystem;
		Graphics::IGraphicsDevice *device;
	};
} // namespace Hush
