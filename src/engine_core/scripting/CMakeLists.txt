# Scripting

add_library(HushCSharp OBJECT src/DotnetHost.cpp src/ScriptingManager.cpp)

target_include_directories(HushCSharp PUBLIC src)

target_link_libraries(HushCSharp PUBLIC HushUtils HushLog coreclr)

set_all_warnings(HushCSharp)