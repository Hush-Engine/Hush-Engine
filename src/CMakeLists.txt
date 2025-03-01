
set(_HUSH_MODULES_LIST "")

include(cmake/utils.cmake)

add_subdirectory(engine_core)

add_subdirectory(editor)

hush_add_library(
        TARGET_NAME HushEngine
        LIB_TYPE STATIC
        SRCS engine_core/src/main.cpp engine_core/src/HushEngine.cpp
        PUBLIC_HEADER_DIRS engine_core/src
)

add_library(Hush::Engine ALIAS HushEngine)

target_link_libraries(HushEngine PUBLIC coreclr
        Hush::Libs
)

hush_add_executable(
        TARGET_NAME HushRuntime
        SRCS engine_core/src/dummy.cpp
)

target_link_libraries(HushRuntime PRIVATE Hush::Engine)

if (WIN32)
    set_target_properties(HushEngine PROPERTIES
            VS_GLOBAL_RunCodeAnalysis false

            # Use visual studio core guidelines
            VS_GLOBAL_EnableMicrosoftCodeAnalysis false
            #VS_GLOBAL_CodeAnalysisRuleSet ${CMAKE_CURRENT_SOURCE_DIR}/foo.ruleset
            #VS_GLOBAL_CodeAnalysisRuleSet ${CMAKE_CURRENT_SOURCE_DIR}/foo.ruleset

            # Use clangtidy
            VS_GLOBAL_EnableClangTidyCodeAnalysis true
            VS_GLOBAL_ClangTidyChecks "-* \"\"--config-file=${CMAKE_CURRENT_SOURCE_DIR}/.clang-tidy --header-filter=.*"
    )
endif ()