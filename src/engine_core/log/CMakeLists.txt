# Log

add_library(HushLog OBJECT src/Logger.cpp)

target_include_directories(HushLog PUBLIC src)

target_link_libraries(HushLog PUBLIC fmt::fmt spdlog::spdlog magic_enum::magic_enum)

set_all_warnings(HushLog)