# Set the source files
set(SOURCES
    src/main.cpp
    src/ScriptingManager.cpp
    src/ScriptingManager.hpp
    src/utils/LibManager.cpp
    src/utils/LibManager.hpp
    src/utils/StringUtils.cpp
    src/utils/StringUtils.hpp
)

add_executable(engine_core ${SOURCES})

target_include_directories(engine_core PRIVATE src src/utils)

if (UNIX)
add_link_options("-ldl")
target_link_libraries(engine_core PUBLIC coreclr dl)
elseif(WIN32)
target_link_libraries(engine_core PUBLIC coreclr)
endif()

target_compile_features(engine_core PRIVATE cxx_std_17)

# Copy assembly-test.runtimeconfig.json to the output directory
add_custom_command(TARGET engine_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/Debug/net8.0/assembly-test.runtimeconfig.json
        $<TARGET_FILE_DIR:engine_core>)

# Copy assembly-test.dll to the output directory
add_custom_command(TARGET engine_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/Debug/net8.0/assembly-test.dll
        $<TARGET_FILE_DIR:engine_core>)