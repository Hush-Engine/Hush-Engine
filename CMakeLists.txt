#Debugging stuff
function(print_files collection)
    foreach(file ${collection})
        message("\t${file}")
    endforeach()
endfunction()

cmake_minimum_required(VERSION 3.26)

project(Hush)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)

add_subdirectory(third_party/coreclr)
add_subdirectory(third_party/imgui)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/Debug/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/Debug/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${PROJECT_BINARY_DIR}/Debug/bin)
