# HushCore

hush_add_library(
        TARGET_NAME HushAppLoader
        LIB_TYPE OBJECT
        SRCS src/IApplication.cpp
             src/ApplicationLoader.cpp
        PUBLIC_HEADER_DIRS src
        PRIVATE_HEADER_DIRS internal
)

add_library(Hush::AppLoader ALIAS HushAppLoader)

target_link_libraries(HushAppLoader PUBLIC HushUtils Hush::Log Hush::Core Hush::Threading)

set_all_warnings(HushAppLoader)
