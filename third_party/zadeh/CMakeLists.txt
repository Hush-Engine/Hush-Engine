message("Including zadeh!!!!")
add_library(zadeh INTERFACE)

target_include_directories(zadeh SYSTEM INTERFACE include)
