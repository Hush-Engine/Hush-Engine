# Utils

find_package(outcome CONFIG REQUIRED)

hush_add_library(
        TARGET_NAME HushUtils
        LIB_TYPE OBJECT
        SRCS src/StringUtils.cpp
             src/LibManager.cpp
             src/filesystem/PathUtils.cpp
             src/SharedLibrary.cpp
        PUBLIC_HEADER_DIRS src
)

add_library(Hush::Utils ALIAS HushUtils)

target_link_libraries(HushUtils PUBLIC HushLog outcome::hl)
if (HUSH_ENABLE_PROFILING)
    target_link_libraries(HushUtils PUBLIC Tracy::TracyClient)
    target_compile_definitions(HushUtils PUBLIC HUSH_ENABLE_PROFILING)
endif()

if (UNIX)
    target_link_libraries(HushUtils PRIVATE dl)
endif ()
