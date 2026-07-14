# Hush::Cooker — offline asset cooking (write path)
# NOT part of HushLibs. Only linked by editor and CLI.

find_package(zstd CONFIG REQUIRED)

find_package(Catch2 CONFIG REQUIRED)
include(Catch)

hush_add_library(
    TARGET_NAME HushCooker
    LIB_TYPE OBJECT
    SRCS src/AssetCooker.cpp
         src/Cookers/ImageCooker.cpp
         src/Cookers/ShaderCooker.cpp
         src/CookedDirectory.cpp
    PUBLIC_HEADER_DIRS src
)
add_library(Hush::Cooker ALIAS HushCooker)

target_sources(HushCooker PUBLIC
    $<TARGET_OBJECTS:HushAssets>
    $<TARGET_OBJECTS:HushRendering>
)

target_link_libraries(HushCooker PUBLIC
    Hush::Assets
    Hush::Rendering
    stb
    zstd::libzstd
)

# CLI executable
hush_add_executable(
    TARGET_NAME HushCookerCli
    SRCS src/CookerMain.cpp
         src/stb_impl.cpp
)
target_link_libraries(HushCookerCli PRIVATE
    Hush::Cooker
    Hush::Assets
    Hush::Filesystem
    Hush::Core
    Hush::Utils
    Hush::Log
    Hush::Threading
    Hush::Rendering
    Hush::Input
)
hush_deploy_runtime_dlls(HushCookerCli)

add_test_target(
    TARGET_NAME HushCookerTest
    ENGINE_TARGET HushCooker
    SRCS tests/ImageCooker.test.cpp
         src/stb_impl.cpp
    HEADER_DIRS tests
)

# add_test_target only links the ENGINE_TARGET (HushCooker) + Hush::Log + Catch2.
# HushCooker forwards HushRendering's objects (WindowRenderer, RenderGraphSystem),
# which need Hush::Input (InputManager) and Hush::Core (Scene/RawQuery). Mirror the
# CLI's link set so the test links. (Guarded: the target only exists when tests are on.)
if (TARGET HushCookerTest)
    target_link_libraries(HushCookerTest PRIVATE
        Hush::Assets Hush::Filesystem Hush::Core Hush::Utils
        Hush::Threading Hush::Input Hush::Rendering)
endif()
