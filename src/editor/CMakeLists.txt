# Editor

add_executable(HushEditor
        src/ContentPanel.cpp
        src/EditorApp.cpp
        src/HierarchyPanel.cpp
        src/ScenePanel.cpp
        src/TitleBarMenuPanel.cpp
        src/UI.cpp
        src/EditorApp.cpp
)

target_link_libraries(HushEditor PRIVATE HushEngine HushAppLoader imgui)