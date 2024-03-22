include(cmake/deps.cmake)

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
    src/utils/Platform.hpp
    src/utils/Assertions.hpp
    src/utils/filesystem/PathUtils.hpp
    src/utils/filesystem/PathUtils.cpp
    src/rendering/WindowRenderer.hpp
    src/rendering/WindowRenderer.cpp
    src/rendering/Renderer.hpp
    src/rendering/VulkanRenderer.hpp
    src/rendering/VulkanRenderer.cpp
    src/rendering/imgui/IImGuiForwarder.hpp
    src/rendering/imgui/VulkanImGuiForwarder.hpp
    src/rendering/imgui/VulkanImGuiForwarder.cpp
    src/input/definitions/KeyStates.hpp
    src/input/definitions/KeyData.hpp
    src/input/definitions/KeyCode.hpp
    src/input/definitions/MouseButton.hpp
    src/input/definitions/MouseData.hpp
    src/input/InputManager.hpp
    src/input/InputManager.cpp
    src/editor/IEditorPanel.hpp
    src/editor/ScenePanel.hpp
    src/editor/MainPanel.hpp
    src/editor/MainPanel.cpp
    src/log/Logger.hpp
    src/log/Logger.cpp
)

add_executable(engine_core ${SOURCES})

target_include_directories(engine_core PRIVATE src src)

#set(ENGINE_LIBS coreclr SDL2::SDL2 magic_enum::magic_enum fmt::fmt spdlog::spdlog volk::volk_headers vk-bootstrap::vk-bootstrap glm::glm)
set(ENGINE_LIBS coreclr)

if (UNIX)
    set(ENGINE_LIBS ${ENGINE_LIBS} dl)
    target_compile_options(engine_core PRIVATE -Wall -Wextra -Wpedantic)
elseif(WIN32)
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

    target_compile_options(engine_core PRIVATE /W4 /WX)
endif()
target_link_libraries(engine_core PUBLIC ${ENGINE_LIBS})

target_compile_features(engine_core PRIVATE cxx_std_17)
#Copy the .NET runtime configuration
set(RUNTIME_CONFIG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assembly-test/bin)
#Check if we're in a single or multiple config generator
if (CMAKE_CONFIGURATION_TYPES)
    set(BUILD_TYPE $<CONFIG>)
else()
    set(BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()
