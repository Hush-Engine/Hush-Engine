
# Editor

hush_add_executable(
        TARGET_NAME HushEditor
        SRCS src/ContentPanel.cpp
             src/SystemSelection.cpp
             src/EditorApp.cpp
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

# Add compile definition for the editor target
target_compile_definitions(HushEditor PRIVATE
    "HUSH_DEFAULT_PROJECT_DIR=R\"(${ESCAPED_PROJECT_ROOT})\""
)

target_link_libraries(HushEditor PRIVATE Hush::Engine imgui imguizmo)

hush_deploy_runtime_dlls(HushEditor)

if (HUSH_ENABLE_MIMALLOC)
    hush_minject_target(HushEditor)
endif()
