#pragma once

#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "ISystem.hpp"
#include "Query.hpp"
#include "Shared/EditorCamera.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/glm.hpp>

namespace Hush
{
	namespace Graphics {
		class IShaderModule;
		class IGraphicsPipeline;
		class IBindGroupLayout;
		class IBindGroup;
	}

	namespace RenderGraph {
		class RenderGraph;
	}

	
	struct ViewUniforms
	{
		glm::mat4 invviewproj;
		glm::vec4 pos;
		glm::vec4 forward;
		glm::vec4 up;
		glm::vec4 right;
		glm::vec2 resolution;
		float farPlane;
		uint8_t padding[35];
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

	private:

		// Needs to be static to access private members
		static void BuildGridPassFunction(Hush::RenderGraph::RenderGraph& graph, Hush::RenderingSystem* self);

		Query<const MeshReference, const WorldTransform> m_renderableTargetsQuery;
		Query<EditorCamera> m_editorCameraQuery;

		ViewUniforms m_cachedViewUniforms;
		glm::u32vec2 m_cachedViewportSize{1, 1}; // Min dimensions set to 1 to avoid breaking graphics APIs
		std::unique_ptr<Graphics::IShaderModule> m_vertModule;
		std::unique_ptr<Graphics::IShaderModule> m_fragModule;
		std::unique_ptr<Graphics::IGraphicsPipeline> m_gridPipeline;
		std::unique_ptr<Graphics::IBindGroupLayout> m_gridBindGroupLayout;
		std::unique_ptr<Graphics::IBindGroup> m_gridBindGroup;
		std::unique_ptr<Graphics::IGraphicsBuffer> m_gridUniformBuffer;
	};
} // namespace Hush
