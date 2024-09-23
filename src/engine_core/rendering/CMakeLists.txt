# Rendering

add_library(HushRendering OBJECT
        src/WindowRenderer.cpp
        src/Vulkan/VulkanVertexBuffer.cpp
        src/Vulkan/VulkanRenderer.cpp
        src/Vulkan/VulkanPipelineBuilder.cpp
        src/Vulkan/VkDescriptors.cpp
        src/Shared/Camera.cpp
        src/ImGui/VulkanImGuiForwarder.cpp
)

target_include_directories(HushRendering PUBLIC src)

target_link_libraries(HushRendering PUBLIC
        SDL2::SDL2
        imgui
        Vulkan::Vulkan
        vk-bootstrap::vk-bootstrap
        glm::glm
        HushLog
        HushUtils
        HushInput
)

set_all_warnings(HushRendering)