# Hush::Assets — cooked-asset formats & read-only VFS backend
# Part of HushLibs (engine_core), linked by runtime & editor.

hush_add_library(
    TARGET_NAME HushAssets
    LIB_TYPE OBJECT
    SRCS src/HAsset.cpp
         src/HushPak.cpp
         src/HShader.cpp
         src/HMeta.cpp
         src/PakFileSystem.cpp
    PUBLIC_HEADER_DIRS src
)
add_library(Hush::Assets ALIAS HushAssets)

target_link_libraries(HushAssets PUBLIC
    Hush::Filesystem
    Hush::Core
    Hush::Utils
    zstd::libzstd
)

add_test_target(
    TARGET_NAME HushAssetsTest
    ENGINE_TARGET HushAssets
    SRCS tests/HAsset.test.cpp
         tests/HushPak.test.cpp
         tests/HShader.test.cpp
         tests/HMeta.test.cpp
    HEADER_DIRS tests
)
