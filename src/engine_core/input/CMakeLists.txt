# Input

add_library(HushInput OBJECT src/InputManager.cpp)

target_include_directories(HushInput PUBLIC src)

target_link_libraries(HushInput PUBLIC magic_enum::magic_enum HushLog)

set_all_warnings(HushInput)