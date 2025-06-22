# Rendering
# This is the default renderer and should be switched accordingly
add_compile_definitions(HUSH_VULKAN_IMPL)

hush_add_library(
        TARGET_NAME HushRendering
        LIB_TYPE OBJECT
        SRCS src/WindowRenderer.cpp
             src/Shared/GpuAllocatedBuffer.cpp
             src/Vulkan/VulkanRenderer.cpp
             src/Vulkan/GltfMetallicRoughness.cpp
             src/Vulkan/VulkanPipelineBuilder.cpp
             src/Vulkan/VulkanFullScreenPass.cpp
             src/Shared/ShaderMaterial.cpp
             src/Shared/Mesh.cpp
             src/Vulkan/VulkanLoader.cpp
             src/Vulkan/VkDescriptors.cpp
             src/Vulkan/VulkanSwapchain.cpp
             src/Vulkan/ShaderModuleLoader.cpp
             src/Shared/Camera.cpp
             src/Shared/ImageTexture.cpp
             src/Shared/EditorCamera.cpp
             src/Shared/DefaultImages.cpp
             src/Shared/GltfLoadFunctions.cpp
             src/ImGui/VulkanImGuiForwarder.cpp
        PUBLIC_HEADER_DIRS src
)

add_library(Hush::Rendering ALIAS HushRendering)

target_link_libraries(HushRendering PUBLIC
        SDL2::SDL2
        imgui
        stb
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glm::glm
        unofficial::spirv-reflect
        fastgltf::fastgltf
        HushLog
        Hush::Core
        HushUtils
        HushInput
)

