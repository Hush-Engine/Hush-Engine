# Rendering

# To enable a specific backend renderer, use the following flags:
# - HUSH_VULKAN_IMPL
# - HUSH_WEBGPU_IMPL

hush_add_library(
        TARGET_NAME HushRendering
        LIB_TYPE OBJECT
        SRCS src/WindowRenderer.cpp
             src/Shared/GpuAllocatedBuffer.cpp
             src/Shared/ShaderMaterial.cpp
             src/Shared/Mesh.cpp

             src/Shared/Camera.cpp
             src/Shared/ImageTexture.cpp
             src/Shared/EditorCamera.cpp
             src/Shared/DefaultImages.cpp

             # WebGPU sources
             src/WebGPU/WebGPURenderer.cpp

             # RenderGraph
             src/RenderGraph/ResourceNode.cpp
             src/RenderGraph/RenderPassNode.cpp
             src/RenderGraph/RenderGraph.cpp
        PUBLIC_HEADER_DIRS src
)

if (HUSH_VULKAN_IMPL)
    target_sources(HushRendering PRIVATE
    PRIVATE
        src/Vulkan/VkDescriptors.cpp
        src/Vulkan/VulkanSwapchain.cpp
        src/Vulkan/ShaderModuleLoader.cpp
        src/Vulkan/VulkanRenderer.cpp
        src/Vulkan/GltfMetallicRoughness.cpp
        src/Vulkan/VulkanPipelineBuilder.cpp
        src/Vulkan/VulkanFullScreenPass.cpp
        src/ImGui/VulkanImGuiForwarder.cpp
        src/Shared/Camera.cpp
        src/Shared/ImageTexture.cpp
        src/Shared/EditorCamera.cpp
        src/Shared/DefaultImages.cpp
        src/ImGui/VulkanImGuiForwarder.cpp
    )

    target_link_libraries(HushRendering INTERFACE
            Vulkan::Vulkan
            vk-bootstrap::vk-bootstrap
            unofficial::spirv-reflect
    )
endif()

if (HUSH_WEBGPU_IMPL)
    target_sources(HushRendering PRIVATE
    PRIVATE
        src/WebGPU/WebGPURenderer.cpp
    )

    target_link_libraries(HushRendering INTERFACE
            webgpu
            sdl3webgpu
    )
endif()

add_library(Hush::Rendering ALIAS HushRendering)

target_link_libraries(HushRendering PUBLIC
        SDL3::SDL3
        imgui
        imguizmo
        stb
        glm::glm
        fastgltf::fastgltf
        HushLog
        Hush::Core
        HushUtils
        HushInput
        webgpu
)

if (NOT HUSH_RENDERER_IMPL)
    target_compile_definitions(HushRendering PRIVATE HUSH_WEBGPU_IMPL)
endif()
