cmake_minimum_required(VERSION 3.26)

project(test)

include(cmake/clang-tidy.cmake)
include(cmake/format.cmake)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)

add_subdirectory(third_party/coreclr)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
