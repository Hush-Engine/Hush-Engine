  

add_executable(embedding_test src/main.cpp)

target_link_libraries(embedding_test PUBLIC coreclr)

# Copy assembly-test.runtimeconfig.json to the output directory
add_custom_command(TARGET embedding_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/Debug/net8.0/assembly-test.runtimeconfig.json
        $<TARGET_FILE_DIR:embedding_test>)

# Copy assembly-test.dll to the output directory
add_custom_command(TARGET embedding_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/Debug/net8.0/assembly-test.dll
        $<TARGET_FILE_DIR:embedding_test>)