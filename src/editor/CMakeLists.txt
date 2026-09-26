
# Editor

hush_add_executable(
        TARGET_NAME HushEditor
        SRCS src/ContentPanel.cpp
             src/SystemSelection.cpp
             src/CookedFileSystem.cpp
             src/CookerService.cpp
             src/EditorApp.cpp
             src/FileWatcher.cpp
             src/HierarchyPanel.cpp
             src/NotificationPanel.cpp
             src/InspectorPanel.cpp
             src/ScenePanel.cpp
             src/StatsPanel.cpp
             src/CommandPanel.cpp
             src/TitleBarMenuPanel.cpp
             src/systems/CommandSystem.cpp
             src/systems/EditorCameraSystem.cpp
             src/systems/RenderingSystem.cpp
             src/UI.cpp
             src/DebugTooltip.cpp
        PUBLIC_HEADER_DIRS src
)

# Allow HUSH_DEFAULT_PROJECT_ROOT to be passed as a CLI argument:
#   cmake -DHUSH_DEFAULT_PROJECT_ROOT=/path/to/project ..
option(HUSH_DEFAULT_PROJECT_ROOT "Path to project root" "")


if(HUSH_DEFAULT_PROJECT_ROOT STREQUAL "")
    if(DEFINED ENV{HUSH_DEFAULT_PROJECT_ROOT})
        set(HUSH_DEFAULT_PROJECT_ROOT $ENV{HUSH_DEFAULT_PROJECT_ROOT})
    else()
        # Fallback to the source directory
        set(HUSH_DEFAULT_PROJECT_ROOT "${CMAKE_SOURCE_DIR}/SampleProject")
    endif()
endif()

string(REPLACE ")" "\\)" ESCAPED_PROJECT_ROOT "${HUSH_DEFAULT_PROJECT_ROOT}")

# Add compile definition for the editor target. TODO: eventually, this should be picked up from the project settings file
target_compile_definitions(HushEditor PRIVATE
    "HUSH_DEFAULT_PROJECT_DIR=R\"(${ESCAPED_PROJECT_ROOT})\""
)

target_compile_definitions(HushEditor PRIVATE
    "HUSH_ENGINE_RES_DIR=R\"(${CMAKE_SOURCE_DIR})\""
)

target_link_libraries(HushEditor PRIVATE Hush::Engine imgui imguizmo imguifiledialog)
# thomasmonkman-filewatch (header-only, no CMake config)
find_path(FILEWATCH_INCLUDE_DIR "thomasmonkman-filewatch/FileWatch.hpp")
if(NOT FILEWATCH_INCLUDE_DIR)
    message(FATAL_ERROR "filewatch headers not found (thomasmonkman-filewatch/FileWatch.hpp). "
                        "Install the thomasmonkman-filewatch package (e.g. via vcpkg) and re-run CMake.")
endif()
target_include_directories(HushEditor PRIVATE ${FILEWATCH_INCLUDE_DIR})
message(STATUS "Found filewatch: ${FILEWATCH_INCLUDE_DIR}")

target_link_libraries(HushEditor PRIVATE Hush::Engine Hush::Cooker imgui imguizmo)

# EditorApp.cpp includes the generated HushBindings.cpp directly, so the
# bindings must be generated before the editor compiles.
add_dependencies(HushEditor GenerateHushBindings)

hush_deploy_runtime_dlls(HushEditor)

if (HUSH_ENABLE_MIMALLOC)
    hush_minject_target(HushEditor)
endif()
