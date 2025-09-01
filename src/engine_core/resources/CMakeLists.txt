# Resource classes and resource manager-things.


hush_add_library(
        TARGET_NAME HushResources
        LIB_TYPE OBJECT
        SRCS src/ResourceManager.cpp
        src/Loaders/VulkanLoader.cpp
        src/Loaders/GltfLoadFunctions.cpp
        PUBLIC_HEADER_DIRS src
)
add_library(Hush::Resources ALIAS HushResources)

target_link_libraries(HushResources PUBLIC Hush::Utils Hush::Log Hush::Filesystem Hush::Rendering Hush::Threading)
