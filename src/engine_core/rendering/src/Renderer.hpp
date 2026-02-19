
/*! \file Renderer.hpp
	\author Alan Ramirez Herrera
	\date 2024-03-03
	\brief Abstract renderer
*/

#pragma once

#include "Components/WorldTransform.hpp"
#include "Shared/DefaultImages.hpp"
#include "Shared/EditorCamera.hpp"
#include "Shared/GpuAllocatedImage.hpp"
#include "Shared/Mesh.hpp"
#include "../../resources/src/Ref.hpp"
#include "Shared/Types/Color.hpp"
#include "Shared/Types/ImageExtent3D.hpp"
#include <SDL2/SDL.h>
#include <cstdint>
#include <functional>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/vec3.hpp>
#include <memory>
#include <string_view>

namespace Hush
{
	class Scene;
	struct DirectionalLight;

	/// @brief Renderer type enumeration
	/// Currently, Hush uses wgpu on desktop as its backend implementation. This means that
	/// we rely on wgpu for every renderer (except Vulkan).
	enum class ERenderingBackend
	{
		/// @brief Vulkan renderer
		Vulkan,
		/// @brief DirectX 12 renderer.
		D3D12,
		/// @brief Metal renderer.
		Metal,
		/// @brief WebGL renderer.
		WebGL,
		/// @brief WebGPU renderer.
		WebGPU,
	};

	/// @brief A common interface for renderers, Hush supports many graphics APIs, and this is the interface
	/// that allows us to standardize all of them...
	/// All renderers MUST bind to SDL and ImGUI, the latter can be done through the IImGuiForwarder interface
	class IRenderer
	{
	public:
		IRenderer(void *windowContext, ERenderingBackend type)
		{
			HUSH_UNUSED(windowContext);
			HUSH_UNUSED(type);
		}

		IRenderer(const IRenderer &) = delete;
		IRenderer &operator=(const IRenderer &) = delete;
		IRenderer(IRenderer &&) = delete;
		IRenderer &operator=(IRenderer &&) = delete;

		virtual ~IRenderer() = default;

		virtual void CreateSwapChain(uint32_t width, uint32_t height) = 0;

		virtual void SetActiveScene(Scene *scene) = 0;

		virtual void InitImGui() = 0;

		/// @brief Must be called before every new frame to clear out all the stale mesh and transform data (this is the
		/// responsibility of the RenderingSystem)
		virtual void ClearDrawContext() = 0;

		virtual void PushMesh(const WorldTransform *xform, const Mesh *mesh) = 0;

		virtual void DestroyMesh(const std::string_view &name) = 0;

		virtual void Draw(float delta) = 0;

		/// @brief Each renderer will have to implement a way of updating all the objects
		/// inside of the scene, these are instances of the IRenderableNode, which is a common interface
		/// for all renderers, but additional render data (i.e drawContext) might be needed by their underlying
		/// implementation (see VulkanMeshNode for an example)
		virtual void UpdateSceneObjects(float delta) = 0;

		/// @brief Initializes all the internal structures needed to begin rendering, call after a swapchain has been
		/// created!
		virtual void InitRendering() = 0;

		virtual void NewUIFrame() const noexcept = 0;

		virtual void EndUIFrame() const noexcept = 0;

		virtual void HandleEvent(const SDL_Event *event) noexcept = 0;

		virtual GpuAllocatedImage CreateImage(const void *data, const ImageExtent3D &size, Color::EFormat format,
											  uint32_t usage, bool mipmapped = false) = 0;

		virtual void DestroyImage(GpuAllocatedImage *image) = 0;

		virtual void AddToDeletionQueue(std::function<void()> &&deleteFunc) = 0;

		[[nodiscard]]
		virtual const DefaultImageProvider *GetDefaultImageProvider() const noexcept = 0;

		[[nodiscard]]
		virtual void *GetWindowContext() const noexcept = 0;

		// TODO: Figure out a better system to get the editor camera
		[[nodiscard]]
		virtual const EditorCamera &GetEditorCamera() const noexcept = 0;
		virtual EditorCamera *GetEditorCamera() noexcept = 0;
	};
} // namespace Hush
