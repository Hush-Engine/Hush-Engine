cmake_minimum_required(VERSION 3.26)

project(test)

include(cmake/clang-tidy.cmake)
include(cmake/format.cmake)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(src)

add_subdirectory(third_party/coreclr)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Add clang-tidy files
file(GLOB_RECURSE ALL_SOURCE_FILES src/*.cpp src/*.hpp)

# Add clang-tidy target
add_custom_target(
  tidy
  COMMAND
    python ${PROJECT_SOURCE_DIR}/scripts/run-clang-tidy.py -p
    ${CMAKE_BINARY_DIR} -config-file ${PROJECT_SOURCE_DIR}/.clang-tidy
    ${PROJECT_SOURCE_DIR}/src -quiet -header-filter='.*')

add_custom_target(
  tidy-fix
  COMMAND
    python ${PROJECT_SOURCE_DIR}/scripts/run-clang-tidy.py -p
    ${CMAKE_BINARY_DIR} -config-file ${PROJECT_SOURCE_DIR}/.clang-tidy -fix
    ${PROJECT_SOURCE_DIR}/src -quiet -header-filter='.*')

# Add clang-format target
file(GLOB_RECURSE ALL_SOURCE_FILES src/*.cpp src/*.hpp)

add_custom_target(format COMMAND clang-format -style=file -i
                                 ${ALL_SOURCE_FILES})
