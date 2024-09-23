# Utils

add_library(HushUtils OBJECT
        src/StringUtils.cpp
        src/LibManager.cpp
        src/filesystem/PathUtils.cpp
        src/SharedLibrary.cpp
)

target_link_libraries(HushUtils PUBLIC HushLog)

target_include_directories(HushUtils PUBLIC src)

if (UNIX)
    target_link_libraries(HushUtils PRIVATE dl)
endif ()

set_all_warnings(HushUtils)