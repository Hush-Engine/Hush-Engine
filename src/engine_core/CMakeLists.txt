# Set the source files
set(SOURCES
    src/main.cpp
    src/scripting/ScriptingManager.cpp
    src/scripting/ScriptingManager.hpp
    src/scripting/DotnetHost.hpp
    src/scripting/DotnetHost.cpp
    src/utils/LibManager.cpp
    src/utils/LibManager.hpp
    src/utils/StringUtils.cpp
    src/utils/StringUtils.hpp
    src/utils/filesystem/PathUtils.hpp
    src/utils/filesystem/PathUtils.cpp
    src/Logger.hpp
    src/rendering/WindowRenderer.hpp
    src/rendering/WindowRenderer.cpp
)

add_executable(engine_core ${SOURCES})

target_include_directories(engine_core PRIVATE src src/utils)

if (UNIX)
add_link_options("-ldl")
target_link_libraries(engine_core PUBLIC coreclr dl)
elseif(WIN32)
target_link_libraries(engine_core PUBLIC coreclr)
set_target_properties(engine_core PROPERTIES
    VS_GLOBAL_RunCodeAnalysis false

    # Use visual studio core guidelines
    VS_GLOBAL_EnableMicrosoftCodeAnalysis false
    #VS_GLOBAL_CodeAnalysisRuleSet ${CMAKE_CURRENT_SOURCE_DIR}/foo.ruleset
    #VS_GLOBAL_CodeAnalysisRuleSet ${CMAKE_CURRENT_SOURCE_DIR}/foo.ruleset

    # Use clangtidy
    VS_GLOBAL_EnableClangTidyCodeAnalysis true
    VS_GLOBAL_ClangTidyChecks "-* \"\"--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy --header-filter=.*" 
)
endif()

target_compile_features(engine_core PRIVATE cxx_std_17)

# Copy assembly-test.runtimeconfig.json to the output directory
add_custom_command(TARGET engine_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/${CMAKE_BUILD_TYPE}/net8.0/assembly-test.runtimeconfig.json
        $<TARGET_FILE_DIR:engine_core>)

# Copy assembly-test.dll to the output directory
add_custom_command(TARGET engine_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin/${CMAKE_BUILD_TYPE}/net8.0/assembly-test.dll
        $<TARGET_FILE_DIR:engine_core>)