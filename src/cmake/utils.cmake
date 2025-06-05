# utils.cmake
# Alan Ramirez
# 2024-09-22
# CMake utils

if (MSVC)
    # Check if the file exists
    if (NOT EXISTS "${CMAKE_BINARY_DIR}/hush-reflection.exe")
        set(EXPECTED_SHA256 "ba891ae7ef960d06d0637489a0dc5b32d8cf6295df6b94fc228858ac82efcb50")
        file(
                DOWNLOAD "https://github.com/Hush-Engine/hush-llvm/releases/download/v0.1.0/hush-reflection.exe"
                "${CMAKE_BINARY_DIR}/hush-reflection.exe"
                STATUS download_status
                EXPECTED_HASH SHA256=${EXPECTED_SHA256}
        )
        if (NOT download_status EQUAL 0)
            message(FATAL_ERROR "Failed to download hush-reflection.exe: ${download_status}")
        endif ()
    else ()
        message(STATUS "hush-reflection.exe already exists, skipping download.")
    endif ()

    set(HUSH_REFLECTION_BIN "${CMAKE_BINARY_DIR}/hush-reflection.exe")
endif ()


# Set all warnings for the target
macro(set_all_warnings target)
    if (UNIX)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    elseif (WIN32)
        target_compile_options(${target} PRIVATE /W4 /WX)
    endif ()
endmacro()

# TARGET_NAME: Name of the target
# PUBLIC_HEADER_DIRS: Public header directories for the target
# PRIVATE_HEADER_DIRS: Private header directories for the target
# REFLECTION_SUBDIR: Subdirectory to append to the current working directory.
macro(enable_reflection)
    cmake_parse_arguments(REFLECT "" "TARGET_NAME" "PUBLIC_HEADER_DIRS;PRIVATE_HEADER_DIRS;REFLECTION_SUBDIR" ${ARGN})

    # Glob all header files in the public and private directories
    if (REFLECT_PUBLIC_HEADER_DIRS)
        file(GLOB_RECURSE PUBLIC_HEADERS_FILES ${REFLECT_PUBLIC_HEADER_DIRS}/*.hpp)
    endif ()
    if (REFLECT_PRIVATE_HEADER_DIRS)
        file(GLOB_RECURSE PRIVATE_HEADERS_FILES ${REFLECT_PRIVATE_HEADER_DIRS}/*.hpp)
    endif ()

    # From the PUBLIC_HEADERS_FILES and PRIVATE_HEADERS_FILES, we need to remove the files that match .hushgen.hpp files.
    foreach (header IN LISTS PUBLIC_HEADERS_FILES PRIVATE_HEADERS_FILES)
        if (header MATCHES "\\.hushgen\\.hpp$")
            list(REMOVE_ITEM PUBLIC_HEADERS_FILES ${header})
            list(REMOVE_ITEM PRIVATE_HEADERS_FILES ${header})
        endif ()
    endforeach ()

    # Get the target sources
    get_target_property(LIB_SRCS ${REFLECT_TARGET_NAME} SOURCES)

    if (NOT LIB_SRCS)
        message(FATAL_ERROR "No sources found for target ${REFLECT_TARGET_NAME}. Please ensure the target has source files.")
    endif ()

    # Convert the sources to absolute paths
    set(LIB_SRCS_ABSOLUTE "")
    foreach (src IN LISTS LIB_SRCS)
        get_filename_component(ABSOLUTE_SRC ${src} ABSOLUTE)
        set(LIB_SRCS_ABSOLUTE ${LIB_SRCS_ABSOLUTE} ${ABSOLUTE_SRC})
    endforeach ()

    set(WORKING_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    if (REFLECT_REFLECTION_SUBDIR)
        set(WORKING_DIR ${WORKING_DIR}/${REFLECT_REFLECTION_SUBDIR})
    endif ()

    add_custom_command(
            OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${REFLECT_TARGET_NAME}.hushgen.cpp
            COMMAND ${HUSH_REFLECTION_BIN}
            ARGS -p ${CMAKE_BINARY_DIR}/compile_commands.json --output-stamp=${CMAKE_CURRENT_BINARY_DIR}/${REFLECT_TARGET_NAME}.hushgen.cpp ${LIB_SRCS_ABSOLUTE}
            DEPENDS ${PUBLIC_HEADERS_FILES} ${PRIVATE_HEADERS_FILES} ${LIB_SRCS_ABSOLUTE} ${HUSH_REFLECTION_BIN}
            WORKING_DIRECTORY ${WORKING_DIR}
            VERBATIM
    )

    target_sources(${REFLECT_TARGET_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${REFLECT_TARGET_NAME}.hushgen.cpp)

endmacro()

# Helper function to add a hush library. Hush libraries by default are OBJECT libraries.
# TARGET_NAME: Name of the library target
# LIB_TYPE: Type of the library (OBJECT, STATIC, SHARED)
# SRCS: Source files for the library
# PUBLIC_HEADER_DIRS: Public header directories for the library
# PRIVATE_HEADER_DIRS: Private header directories for the library
# ENABLE_REFLECTION: Whether to enable reflection for the library
macro(hush_add_library)
    cmake_parse_arguments(LIB "" "TARGET_NAME;LIB_TYPE" "SRCS;PUBLIC_HEADER_DIRS;PRIVATE_HEADER_DIRS;ENABLE_REFLECTION" ${ARGN})
    add_library(${LIB_TARGET_NAME} ${LIB_LIB_TYPE} ${LIB_SRCS})
    target_include_directories(${LIB_TARGET_NAME} PUBLIC ${LIB_PUBLIC_HEADER_DIRS})
    target_include_directories(${LIB_TARGET_NAME} PRIVATE ${LIB_PRIVATE_HEADER_DIRS})
    set_all_warnings(${LIB_TARGET_NAME})

    if (${HUSH_ENABLE_LTO})
        set_property(TARGET ${LIB_TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif ()
    target_link_options(${LIB_TARGET_NAME} PRIVATE ${HUSH_CPU_FLAGS})
    target_compile_definitions(${LIB_TARGET_NAME} PUBLIC GLM_FORCE_XYZW_ONLY)

    if (${LIB_ENABLE_REFLECTION})
        enable_reflection(
                TARGET_NAME ${LIB_TARGET_NAME}
                PUBLIC_HEADER_DIRS ${LIB_PUBLIC_HEADER_DIRS}
                PRIVATE_HEADER_DIRS ${LIB_PRIVATE_HEADER_DIRS}
        )
    endif ()

endmacro()

# Helper function to add a hush executable
# TARGET_NAME: Name of the executable target
# SRCS: Source files for the executable
# PUBLIC_HEADER_DIRS: Public header directories for the library
# PRIVATE_HEADER_DIRS: Private header directories for the library
# ENABLE_REFLECTION: Whether to enable reflection for the executable
macro(hush_add_executable)
    cmake_parse_arguments(EXE "" "TARGET_NAME" "SRCS;PUBLIC_HEADER_DIRS;PRIVATE_HEADER_DIRS;ENABLE_REFLECTION" ${ARGN})
    add_executable(${EXE_TARGET_NAME} ${EXE_SRCS})
    target_include_directories(${EXE_TARGET_NAME} PRIVATE ${EXE_PUBLIC_HEADER_DIRS} ${EXE_PRIVATE_HEADER_DIRS})
    set_all_warnings(${EXE_TARGET_NAME})

    if (${HUSH_ENABLE_LTO})
        set_property(TARGET ${EXE_TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    endif ()
    target_compile_definitions(${EXE_TARGET_NAME} PUBLIC GLM_FORCE_XYZW_ONLY)

    if (${EXE_ENABLE_REFLECTION})
        enable_reflection(
                TARGET_NAME ${EXE_TARGET_NAME}
                PUBLIC_HEADER_DIRS ${EXE_PUBLIC_HEADER_DIRS}
                PRIVATE_HEADER_DIRS ${EXE_PRIVATE_HEADER_DIRS}
        )
    endif ()
endmacro()

# Adds a test target to the project
# TARGET_NAME: Name of the test target
# ENGINE_TARGET: Engine target to link against
# SRCS: Source files for the test
# HEADER_DIR: Header directories for the test
# ENABLE_REFLECTION: Whether to enable reflection for the test
macro(add_test_target)
    if (HUSH_ENABLE_TESTS)
        cmake_parse_arguments(TEST "" "TARGET_NAME;ENGINE_TARGET" "SRCS;HEADER_DIRS;ENABLE_REFLECTION" ${ARGN})
        add_executable(${TEST_TARGET_NAME} ${TEST_SRCS})
        target_include_directories(${TEST_TARGET_NAME} PRIVATE ${TEST_HEADER_DIRS})
        target_link_libraries(${TEST_TARGET_NAME} PRIVATE ${TEST_ENGINE_TARGET} HushLog Catch2::Catch2WithMain)
        set_all_warnings(${TEST_TARGET_NAME})

        catch_discover_tests(${TEST_TARGET_NAME})

        if (${HUSH_ENABLE_LTO})
            set_property(TARGET ${TEST_TARGET_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        endif ()

        if (${TEST_ENABLE_REFLECTION})
            enable_reflection(
                    TARGET_NAME ${TEST_TARGET_NAME}
                    PRIVATE_HEADER_DIRS ${TEST_HEADER_DIRS}
                    REFLECTION_SUBDIR tests
            )
        endif ()
    else ()
        return()
    endif ()
endmacro()

# add_hush_module is a helper function to add a module to the project
# A module is an object library meant to be linked with hush static library.
# MODULE_NAME: Name of the module
# SRCS: Source files for the module
# HEADER_DIRS: Header directories for the module
# DEFAULT_ENABLED: Default enabled state for the module
function(add_hush_module)
    cmake_parse_arguments(MODULE "" "MODULE_NAME" "SRCS;HEADER_DIRS" ${ARGN})

    set(ENABLE_HUSH_MODULE_${MODULE_MODULE_NAME} ${MODULE_DEFAULT_ENABLED} CACHE BOOL "Enable ${MODULE_MODULE_NAME} module")

    if (NOT ENABLE_HUSH_MODULE_${MODULE_MODULE_NAME})
        return()
    endif ()

    add_library(${MODULE_MODULE_NAME} OBJECT ${MODULE_SRCS})
    target_include_directories(${MODULE_MODULE_NAME} PRIVATE ${MODULE_HEADER_DIRS})
    set_all_warnings(${MODULE_MODULE_NAME})

    target_link_libraries(${MODULE_MODULE_NAME} PRIVATE Hush::ModuleLibs)

    set(_HUSH_MODULES_LIST ${_HUSH_MODULES_LIST} ${MODULE_MODULE_NAME} PARENT_SCOPE)
endfunction()