# Hush binding generator

# Generate HushBindings.h and HushBindings.cpp with the host-side exporter.
set(HUSH_BINDING_ARGS -p ${CMAKE_BINARY_DIR}/compile_commands.json
    ${CMAKE_CURRENT_SOURCE_DIR}/bindings.cpp)
if (EMSCRIPTEN)
    # Like reflection generation, parse with host Clang and explicit target
    # includes/defines rather than passing emcc-only flags such as --use-port.
    set(HUSH_BINDING_ARGS ${CMAKE_CURRENT_SOURCE_DIR}/bindings.cpp -- -std=c++20
        "$<LIST:TRANSFORM,$<TARGET_PROPERTY:HushBindings,INCLUDE_DIRECTORIES>,PREPEND,-I>"
        "$<LIST:TRANSFORM,$<TARGET_PROPERTY:HushBindings,COMPILE_DEFINITIONS>,PREPEND,-D>"
    )
endif()

add_custom_target(
        GenerateHushBindings
        COMMAND ${HUSH_EXPORT_BIN} ${HUSH_BINDING_ARGS}
        COMMAND_EXPAND_LISTS
        VERBATIM
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating Hush bindings..."
)

hush_add_library(TARGET_NAME HushBindings
        LIB_TYPE OBJECT
        SRCS bindings.cpp
)

# HushBindings depends on the GenerateHushBindings target to ensure that the bindings are generated before compiling the library

add_dependencies(HushBindings GenerateHushBindings)

add_library(Hush::Bindings ALIAS HushBindings)

target_link_libraries(HushBindings PUBLIC
        Hush::Threading
        Hush::Engine
        Hush::Input
        Hush::Core
        Hush::Modules
        Hush::Memory
)
