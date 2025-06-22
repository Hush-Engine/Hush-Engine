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
        src/UI.cpp
        src/DebugTooltip.cpp
        src/EditorApp.cpp
)

target_link_libraries(HushEditor PRIVATE Hush::Engine imgui imguizmo)
