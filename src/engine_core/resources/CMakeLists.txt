# Resource classes and resource manager-things.


hush_add_library(
        TARGET_NAME HushResources
        LIB_TYPE OBJECT
        SRCS src/Systems/ResourceUploadSystem.cpp
             src/ResourceManager.cpp
             src/Loaders/GltfLoader.cpp
             src/Loaders/HMeshLoader.cpp
             src/Components/MeshReference.hpp
        # SRCS src/ResourceManager.cpp
        # src/Loaders/VulkanLoader.cpp
        src/Loaders/GltfLoadFunctions.cpp
        PUBLIC_HEADER_DIRS src
        ENABLE_REFLECTION ON
)
add_library(Hush::Resources ALIAS HushResources)

target_link_libraries(HushResources PUBLIC Hush::Utils Hush::Log Hush::Filesystem Hush::Rendering Hush::Threading Hush::Assets)
