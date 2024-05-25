include(cmake/deps.cmake)
message("****COMPILING ENGINE SOURCE****")
# Set the source files
message("Getting scripting module...")
file(GLOB_RECURSE SCRIPTING_SRC "src/scripting/*.hpp" "src/scripting/*.cpp")
print_files("${SCRIPTING_SRC}")
message("Getting utils module...")
file(GLOB_RECURSE UTILS_SRC "src/utils/*.hpp" "src/utils/*.cpp")
print_files("${UTILS_SRC}")
message("Getting rendering module...")
file(GLOB_RECURSE RENDERING_SRC "src/rendering/*.hpp" "src/rendering/*.cpp")
print_files("${RENDERING_SRC}")
message("Getting input module...")
file(GLOB_RECURSE INPUT_SRC "src/input/*.hpp" "src/input/*.cpp")
print_files("${INPUT_SRC}")
message("Getting editor module...")
file(GLOB_RECURSE EDITOR_SRC "src/editor/*.hpp" "src/editor/*.cpp")
print_files("${EDITOR_SRC}")

set(SOURCES
    src/main.cpp
    src/HushEngine.hpp
    src/HushEngine.cpp
    ${SCRIPTING_SRC}
    ${UTILS_SRC}
    ${RENDERING_SRC}
    ${INPUT_SRC}
    ${EDITOR_SRC}
    src/log/Logger.hpp
    src/log/Logger.cpp
)

add_executable(engine_core ${SOURCES})

target_include_directories(engine_core PRIVATE src src)

#set(ENGINE_LIBS coreclr imgui SDL2::SDL2 magic_enum::magic_enum fmt::fmt spdlog::spdlog volk::volk_headers vk-bootstrap::vk-bootstrap glm::glm)
set(ENGINE_LIBS coreclr imgui Vulkan::Vulkan)

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
