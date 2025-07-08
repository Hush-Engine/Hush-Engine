# CMakeLists.txt
# Alan Ramirez
# 2025-07-08
# Docs cmake targets


# For the docs, we need to generate docs with doxygen and then with sphinx.
# This is a simple target that will generate the docs. Working directory must be set to the root of the project.
add_custom_target(docs
    COMMAND ${CMAKE_COMMAND} -E echo "Generating documentation..."
    COMMAND doxygen Doxyfile.in
    COMMAND sphinx-build -b html docs/source docs/build
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating documentation with Doxygen and Sphinx"
)