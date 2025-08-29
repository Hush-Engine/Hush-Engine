#pragma once

#include "Entity.hpp"
#include "Shared/GpuAllocatedImage.hpp"
#include "Shared/ImageTexture.hpp"
#include <vector>

namespace Hush {
	class IRenderer;
	class ResourceManager;
	
	class IModelLoader {
	public:

		enum class EError
		{
			None = 0,
			FileNotFound,
			InvalidMeshFile,
			FormatNotSupported
		};
		
		IModelLoader(const IModelLoader &) = default;
		IModelLoader(IModelLoader &&) = default;
		IModelLoader &operator=(const IModelLoader &) = default;
		IModelLoader &operator=(IModelLoader &&) = default;
		virtual ~IModelLoader() = default;

		IModelLoader() = default;

		virtual void SetResourceManager(ResourceManager* resourceManager) = 0;

		[[nodiscard]] virtual ResourceManager* GetResourceManager() const = 0;

		virtual Result<std::vector<Entity>, EError> LoadMeshes(IRenderer *engine, const std::filesystem::path& filePath, Scene *activeScene) = 0;
		
		virtual GpuAllocatedImage LoadTexture(IRenderer* engine, const ImageTexture &texture) = 0;
	};
}
