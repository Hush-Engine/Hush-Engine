# Set the source files
set(SOURCES
    src/main.cpp
    src/HushEngine.hpp
    src/HushEngine.cpp
    src/scripting/ScriptingManager.cpp
    src/scripting/ScriptingManager.hpp
    src/scripting/DotnetHost.hpp
    src/scripting/DotnetHost.cpp
    src/utils/LibManager.cpp
    src/utils/LibManager.hpp
    src/utils/StringUtils.cpp
    src/utils/StringUtils.hpp
    src/utils/EnumUtils.hpp
    src/utils/filesystem/PathUtils.hpp
    src/utils/filesystem/PathUtils.cpp
    src/Logger.hpp
    src/rendering/WindowRenderer.hpp
    src/rendering/WindowRenderer.cpp
    src/input/KeyStates.hpp
    src/input/KeyData.hpp
    src/input/KeyCode.hpp
    src/input/InputManager.hpp
    src/input/InputManager.cpp
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
#Copy the .NET runtime configuration
set(RUNTIME_CONFIG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin)
#Check if we're in a single or multiple config generator
if (CMAKE_CONFIGURATION_TYPES)
    set(BUILD_TYPE $<CONFIG>)
else()
    set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

# Copy assembly-test.runtimeconfig.json to the output directory
add_custom_command(TARGET engine_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${RUNTIME_CONFIG_SOURCE_DIR}/${BUILD_TYPE}/net8.0/assembly-test.runtimeconfig.json
        $<TARGET_FILE_DIR:engine_core>)

# Copy assembly-test.dll to the output directory
add_custom_command(TARGET engine_core POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${RUNTIME_CONFIG_SOURCE_DIR}/${BUILD_TYPE}/net8.0/assembly-test.dll
        $<TARGET_FILE_DIR:engine_core>)