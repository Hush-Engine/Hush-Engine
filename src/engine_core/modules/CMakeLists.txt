# Hush module system

hush_add_library(
        TARGET_NAME HushModules
        LIB_TYPE OBJECT
        SRCS src/ModuleRegistry.cpp
             src/ModuleSystem.cpp
             src/NativeModuleLoader.cpp
        PUBLIC_HEADER_DIRS src
)

add_library(Hush::Modules ALIAS HushModules)

target_link_libraries(HushModules PUBLIC HushCore HushUtils HushLog)

if (HUSH_ENABLE_TESTS AND NOT EMSCRIPTEN)
    add_test_target(
            TARGET_NAME HushModulesTest
            ENGINE_TARGET HushModules
            SRCS tests/Modules.test.cpp
    )

    # Object libraries do not propagate their objects transitively, so the
    # engine libraries are linked directly.
    target_link_libraries(HushModulesTest PRIVATE HushCore HushUtils HushThreading HushMemory)

    # Handwritten native module used to test the module loader. The module
    # is self contained: every engine call it makes is header inline, so it
    # does not need to link against the host process.
    add_library(HushTestModule MODULE tests/TestNativeModule.cpp)
    target_include_directories(HushTestModule PRIVATE src ../core/src ../utils/src
            ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
    set_all_warnings(HushTestModule)
    set_target_properties(HushTestModule PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:HushModulesTest>"
            LIBRARY_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:HushModulesTest>"
    )

    target_compile_definitions(HushModulesTest PRIVATE
            HUSH_TEST_MODULE_NAME="${CMAKE_SHARED_MODULE_PREFIX}HushTestModule${CMAKE_SHARED_MODULE_SUFFIX}")
    add_dependencies(HushModulesTest HushTestModule)

    # Build a module through hush-reflection as an integration test of
    # [[hush::system]], generated descriptors, and the generated C entry point.
    hush_add_library(
            TARGET_NAME HushGeneratedTestModule
            LIB_TYPE MODULE
            SRCS tests/GeneratedModuleSystem.cpp
            PUBLIC_HEADER_DIRS tests
            ENABLE_REFLECTION ON
            MODULE_ENTRY
    )
    target_include_directories(HushGeneratedTestModule PRIVATE src ../core/src ../utils/src
            ${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/include)
    set_target_properties(HushGeneratedTestModule PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:HushModulesTest>"
            LIBRARY_OUTPUT_DIRECTORY "$<TARGET_FILE_DIR:HushModulesTest>")
    add_dependencies(HushModulesTest HushGeneratedTestModule)
    target_compile_definitions(HushModulesTest PRIVATE
            HUSH_GENERATED_TEST_MODULE_NAME="${CMAKE_SHARED_MODULE_PREFIX}HushGeneratedTestModule${CMAKE_SHARED_MODULE_SUFFIX}")
endif ()
