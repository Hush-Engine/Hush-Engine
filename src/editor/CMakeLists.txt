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

# Copy assembly-test.runtimeconfig.json to the output directory
add_custom_command(TARGET HushEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/Debug/net8.0/assembly-test.runtimeconfig.json
        $<TARGET_FILE_DIR:HushEditor>)

# Copy assembly-test.dll to the output directory
add_custom_command(TARGET HushEditor POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/Debug/net8.0/assembly-test.dll
        $<TARGET_FILE_DIR:HushEditor>)