
if (NOT EMSCRIPTEN)

add_executable(Example1
        src/App.cpp
)
target_link_libraries(Example1 PRIVATE Hush::Engine)

# Cook resources into a bundle via hush-cooker CLI
hush_add_resources(
    TARGET Example1
    CONTENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources
    OUTPUT game.hushpak
    COMPRESSION zstd
)

# Copy raw resources for backward compat (non-cooked load path)
add_custom_command(TARGET Example1 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:Example1>/resources
        DEPENDS Example1
        COMMENT "Copying resources to output directory"
)

endif()
