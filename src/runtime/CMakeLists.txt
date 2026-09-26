# Hush runtime player

hush_add_library(
        TARGET_NAME HushRuntimeApp
        LIB_TYPE OBJECT
        SRCS src/RuntimeApplication.cpp
             src/RuntimeManifest.cpp
        PUBLIC_HEADER_DIRS src
)

add_library(Hush::RuntimeApp ALIAS HushRuntimeApp)

target_link_libraries(HushRuntimeApp PUBLIC
        HushEngine
        HushCore
        HushModules
        HushUtils
        HushLog
        HushFilesystem
        HushAssets
)

# RapidJSON is header only, the imported target is not visible from this
# directory so the include path is used directly.
target_include_directories(HushRuntimeApp PUBLIC
        ${CMAKE_SOURCE_DIR}/src/bindings
        ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include
)

add_dependencies(HushRuntimeApp GenerateHushBindings)

if (HUSH_ENABLE_TESTS)
    find_package(Catch2 CONFIG REQUIRED)
    add_test_target(
            TARGET_NAME HushRuntimeTest
            ENGINE_TARGET HushCore
            SRCS tests/RuntimeManifest.test.cpp
                 src/RuntimeManifest.cpp
            HEADER_DIRS src
    )
    target_link_libraries(HushRuntimeTest PRIVATE HushThreading HushMemory)
endif ()
