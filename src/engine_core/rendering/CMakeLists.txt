# Rendering

# To enable a specific backend renderer, use the following flags:
#
# - HUSH_VULKAN_IMPL
# - HUSH_WEBGPU_IMPL

if (NOT HUSH_RENDERER_IMPL)
    set(HUSH_RENDERER_IMPL HUSH_WEBGPU_IMPL HUSH_VULKAN_IMPL)
endif()

hush_add_library(
        TARGET_NAME HushRendering
        LIB_TYPE OBJECT
        SRCS src/Components/Material3D.cpp
             src/WindowRenderer.cpp
             # src/Shared/GpuAllocatedBuffer.cpp
             # src/Shared/ShaderMaterial.cpp
             # src/Shared/Mesh.cpp

             # src/Shared/Camera.cpp
             # src/Shared/ImageTexture.cpp
             # src/Shared/EditorCamera.cpp
             # src/Shared/DefaultImages.cpp

             # RenderGraph
             src/RenderGraph/RenderGraph.cpp
             src/RenderGraph/ResourceHandle.hpp
             src/RenderGraph/RenderGraphExecutor.cpp

             # Systems
             src/Systems/RenderGraphSystem.cpp

             # RHI
             src/RHI/GraphicsResources.cpp
             src/RHI/ShaderCompiler.cpp

        PUBLIC_HEADER_DIRS src
)

if ("HUSH_WEBGPU_IMPL" IN_LIST HUSH_RENDERER_IMPL)
    target_sources(HushRendering
    PRIVATE
        src/WebGPU/webgpu.cpp
        src/WebGPU/WebGPUBuffer.cpp
        src/WebGPU/WebGPUCommandList.cpp
        src/WebGPU/WebGPUCommandQueue.cpp
        src/WebGPU/WebGPUGraphicsDevice.cpp
        src/WebGPU/WebGPUTexture.cpp
        src/WebGPU/WebGPUSampler.cpp
        src/WebGPU/WebGPUBindGroup.cpp
    )

    target_link_libraries(HushRendering PUBLIC
            webgpu
            sdl2webgpu
    )

    target_compile_definitions(HushRendering PUBLIC HUSH_WEBGPU_IMPL)
endif()

add_library(Hush::Rendering ALIAS HushRendering)

target_link_libraries(HushRendering PUBLIC
        SDL2::SDL2
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
        Boost::unordered
        slang::slang
)
