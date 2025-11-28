/*! \file WebGPURenderer.hpp
	\author Alan Ramirez Herrera
	\date 2025-11-16
	\brief WebGPU implementation for rendering
*/
#pragma once

#include "webgpu/webgpu-raii.hpp"
#include "Renderer.hpp"

namespace Hush
{
	class WebGPURenderer final : public IRenderer
	{
	public:
		WebGPURenderer(void *windowContext, ERenderingBackend type);

		~WebGPURenderer() override = default;

		void CreateSwapChain(uint32_t width, uint32_t height) override;

		void SetActiveScene(Scene *scene) override;

		void InitImGui() override;

		/// @brief Must be called before every new frame to clear out all the stale mesh and transform data (this is the
		/// responsibility of the RenderingSystem)
		void ClearDrawContext() override;

		void PushMesh(const WorldTransform *xform, const Mesh *mesh) override;

		void DestroyMesh(const std::string_view &name) override;

		void Draw(float delta) override;

		/// @brief Each renderer will have to implement a way of updating all the objects
		/// inside of the scene, these are instances of the IRenderableNode, which is a common interface
		/// for all renderers, but additional render data (i.e drawContext) might be needed by their underlying
		/// implementation (see VulkanMeshNode for an example)
		void UpdateSceneObjects(float delta) override;

		/// @brief Initializes all the internal structures needed to begin rendering, call after a swapchain has been
		/// created!
		void InitRendering() override;

		void NewUIFrame() const noexcept override;

		void EndUIFrame() const noexcept override;

		void HandleEvent(const SDL_Event *event) noexcept override;

		GpuAllocatedImage CreateImage(const void *data, const ImageExtent3D &size, Color::EFormat format,
									  uint32_t usage, bool mipmapped = false) override;

		void DestroyImage(GpuAllocatedImage *image) override;

		void AddToDeletionQueue(std::function<void()> &&deleteFunc) override;

		[[nodiscard]]
		const DefaultImageProvider *GetDefaultImageProvider() const noexcept override;

		[[nodiscard]]
		void *GetWindowContext() const noexcept override;

		// TODO: Figure out a better system to get the editor camera
		[[nodiscard]]
		const EditorCamera &GetEditorCamera() const noexcept override;
		EditorCamera *GetEditorCamera() noexcept override;

	private:
		wgpu::raii::Instance m_instance;
		wgpu::raii::Adapter m_adapter;
	};
} // namespace Hush
