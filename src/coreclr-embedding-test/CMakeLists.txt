set(SOURCES src/main.cpp src/ScriptingManager.cpp src/ScriptingManager.hpp src/utils/LibManager.cpp src/utils/LibManager.hpp src/utils/StringUtils.cpp src/utils/StringUtils.hpp)

add_executable(embedding_test ${SOURCES})

target_include_directories(embedding_test PRIVATE src src/utils)

if (UNIX)
add_link_options("-ldl")
target_link_libraries(embedding_test PUBLIC coreclr dl)
elseif(WIN32)
target_link_libraries(embedding_test PUBLIC coreclr)
endif()

target_compile_features(embedding_test PRIVATE cxx_std_17)

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