# Renderer

hush_add_library(
        TARGET_NAME HushRenderer
        LIB_TYPE OBJECT
        SRCS src/Hush/Renderer/Default/DefaultRenderer.cpp
        PUBLIC_HEADER_DIRS src
)

add_library(Hush::Renderer ALIAS HushRenderer)

target_link_libraries(HushRenderer PUBLIC
        imgui
        imguizmo
        stb
        glm::glm
        fastgltf::fastgltf
        HushLog
        Hush::Core
        HushUtils
        Hush::Rendering
        Hush::Resources
)
