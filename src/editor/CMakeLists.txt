# Editor

add_executable(HushEditor
        src/ContentPanel.cpp
        src/EditorApp.cpp
        src/HierarchyPanel.cpp
        src/ScenePanel.cpp
        src/StatsPanel.cpp
        src/TitleBarMenuPanel.cpp
        src/UI.cpp
        src/DebugTooltip.cpp
        src/EditorApp.cpp
)

target_link_libraries(HushEditor PRIVATE Hush::Engine imgui)
