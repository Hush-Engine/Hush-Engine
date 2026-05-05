
hush_add_executable(
        TARGET_NAME WasmExample1
        SRCS src/App.cpp
        PUBLIC_HEADER_DIRS src
        RESOURCES resources/cat.jpg
                  # resources/fullscreen_texture.wgsl
)

target_link_libraries(WasmExample1 PRIVATE Hush::Engine)

# Copy resources folder to binary directory
# add_custom_command(TARGET WasmExample1 POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_directory
#         ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:WasmExample1>/resources
#         DEPENDS WasmExample1
#         COMMENT "Copying resources to output directory"
# )
