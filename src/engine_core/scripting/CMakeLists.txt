# Scripting
# TODO: enable in modules.
hush_add_library(
        TARGET_NAME HushScripting
        LIB_TYPE OBJECT
        SRCS src/ScriptingHost.cpp
        PUBLIC_HEADER_DIRS src
)

add_library(Hush::Scripting ALIAS HushScripting)

target_link_libraries(HushScripting PUBLIC HushUtils HushLog)
