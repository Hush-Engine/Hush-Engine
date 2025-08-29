
# Editor

add_executable(HushEditor
        src/ContentPanel.cpp
        src/EditorApp.cpp
        src/HierarchyPanel.cpp
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
        src/EditorApp.cpp
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

target_link_libraries(HushEditor PRIVATE Hush::Engine imgui imguizmo)
