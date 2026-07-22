#pragma once

#include "Components/Material3D.hpp"
#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "Entity.hpp"
#include "ISystem.hpp"
#include "Query.hpp"
#include "RHI/ShaderCompiler.hpp"
#include "Shared/DirectionalLight.hpp"
#include "Shared/EditorCamera.hpp"
#include "Shared/PBRMaterial.hpp"
#include "VirtualFilesystem.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace Hush
{
	namespace Graphics
	{
		class IShaderModule;
		class IGraphicsPipeline;
		class IBindGroupLayout;
		class IBindGroup;
		class IGraphicsTexture;
		class ISampler;
		class ShaderCompiler;
	} // namespace Graphics

	namespace RenderGraph
	{
		class RenderGraph;
	}

	struct GridViewUniforms
	{
		glm::mat4 invViewProj;
		glm::vec4 pos;
		glm::vec2 resolution;
		float farPlane;
		uint8_t padding[2];
	};

	/// @brief Matches ModelData in mesh.slang — per-draw model matrix
	struct ModelData
	{
		glm::mat4 modelMatrix;
	};

	/// @brief Single draw command populated each frame from MeshReference + WorldTransform
	struct MeshDraw
	{
		glm::mat4 modelMatrix;
		Graphics::IGraphicsBuffer *vertexBuffer;
		Graphics::IGraphicsBuffer *indexBuffer;
		uint32_t indexCount;
		uint32_t firstIndex;
		uint32_t dynamicOffset; // byte offset into per-draw model uniform buffer
		Graphics::IBindGroup *materialBindGroup = nullptr;
	};

	class RenderingSystem final : public ISystem
	{
	public:
		using ISystem::ISystem;

		void Init() override;

		void OnShutdown() override;

		void OnUpdate(float delta) override;

		void OnFixedUpdate(float delta) override;

		void OnRender() override;

		void OnPreRender() override;

		void OnPostRender() override;

		[[nodiscard]]
		std::string_view GetName() const override;

		/// @brief Default PBR material descriptor for instancing any other material
		Graphics::Material3DDescriptor &GetPBRDescriptor();

	private:
		static void BuildScenePassFunction(Hush::RenderGraph::RenderGraph &graph, Hush::RenderingSystem *self);

		void SetupGridPipeline(Graphics::IGraphicsDevice *device, VirtualFilesystem *vfs,
							   Graphics::ShaderCompiler *shaderCompiler);

		Graphics::ShaderCompilationResult SetupMeshPipeline(Graphics::IGraphicsDevice *device, VirtualFilesystem *vfs,
															Graphics::ShaderCompiler *shaderCompiler);

		Query<const MeshReference, const WorldTransform> m_renderableTargetsQuery;
		Query<EditorCamera> m_editorCameraQuery;

		GridViewUniforms m_cachedViewUniforms;
		SceneData m_cachedSceneData;
		glm::u32vec2 m_cachedViewportSize{1, 1};

		// Lighting
		Query<DirectionalLight, WorldTransform> m_directionalLightsQuery;

		// Grid rendering
		std::unique_ptr<Graphics::IShaderModule> m_vertModule;
		std::unique_ptr<Graphics::IShaderModule> m_fragModule;
		std::unique_ptr<Graphics::IGraphicsPipeline> m_gridPipeline;
		std::unique_ptr<Graphics::IBindGroupLayout> m_gridBindGroupLayout;
		std::unique_ptr<Graphics::IBindGroup> m_gridBindGroup;
		std::unique_ptr<Graphics::IGraphicsBuffer> m_gridUniformBuffer;

		// Mesh rendering
		Graphics::Material3DDescriptor m_pbrMaterialDescriptor;
		Graphics::ShaderCompilationResult m_pbrCompilationData;
		std::unique_ptr<Graphics::IShaderModule> m_meshVertModule;
		std::unique_ptr<Graphics::IShaderModule> m_meshFragModule;
		std::unique_ptr<Graphics::IGraphicsPipeline> m_meshPipeline;

		std::unique_ptr<Graphics::IBindGroupLayout> m_meshSceneBindGroupLayout;
		std::unique_ptr<Graphics::IBindGroupLayout> m_meshMaterialBindGroupLayout;
		std::unique_ptr<Graphics::IBindGroup> m_meshSceneBindGroup;
		std::unique_ptr<Graphics::IBindGroup> m_meshMaterialBindGroup;

		std::unique_ptr<Graphics::IGraphicsBuffer> m_sceneDataBuffer;
		std::unique_ptr<Graphics::IGraphicsBuffer> m_meshModelBuffer;
		std::unique_ptr<Graphics::IGraphicsBuffer> m_meshMaterialBuffer;
		uint32_t m_meshModelSlotSize = 0;

		std::unique_ptr<Graphics::IGraphicsTexture> m_defaultColorTex;
		std::unique_ptr<Graphics::IGraphicsTexture> m_defaultMetalRoughTex;
		std::unique_ptr<Graphics::IGraphicsTexture> m_defaultNormalTex;
		std::unique_ptr<Graphics::IGraphicsTexture> m_defaultEmissiveTex;
		std::unique_ptr<Graphics::ISampler> m_defaultSampler;

		// This is a terrible map to keep here because we need to delete the entries when the resource manager frees up
		// the pointer
		// ... That is not yet implemented and we should really pay attention to it later on
		struct CachedMaterialBindGroup
		{
			std::unique_ptr<Graphics::IBindGroup> bindGroup;
			// Per-binding texture pointer at creation time, used to detect async upload completion.
			std::unordered_map<uint32_t, Graphics::IGraphicsTexture *> textures;
		};
		std::unordered_map<const Graphics::Material3D *, CachedMaterialBindGroup> m_materialBindGroupCache;

		std::vector<MeshDraw> m_meshDrawList;
	};
} // namespace Hush
