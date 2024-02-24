# Include dependencies
include(cmake/libs.cmake)

# Set the source files
set(SOURCES
    src/main.cpp
    # Scripting sources
    src/core/scripting/DotnetHost.cpp
    src/core/scripting/ScriptingManager.cpp
    # Utils
    src/core/utils/LibManager.cpp
    src/core/utils/StringUtils.cpp
    # Log
    src/core/log/Logger.cpp
)

add_executable(unnamed_engine ${SOURCES})

target_include_directories(unnamed_engine PRIVATE include)

if (UNIX)
    add_link_options("-ldl")
    target_link_libraries(unnamed_engine PUBLIC coreclr dl spdlog::spdlog)
elseif(WIN32)
    target_link_libraries(unnamed_engine PUBLIC coreclr spdlog::spdlog)
endif()

target_compile_features(unnamed_engine PRIVATE cxx_std_17)

# Copy assembly-test.runtimeconfig.json to the output directory
add_custom_command(TARGET unnamed_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/${CMAKE_BUILD_TYPE}/net8.0/publish/assembly-test.runtimeconfig.json
        $<TARGET_FILE_DIR:unnamed_engine>)

# Copy assembly-test.dll to the output directory
add_custom_command(TARGET unnamed_engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/${CMAKE_BUILD_TYPE}/net8.0/publish/assembly-test.dll
        $<TARGET_FILE_DIR:unnamed_engine>)
