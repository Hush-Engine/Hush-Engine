
if (NOT EMSCRIPTEN)

add_executable(Example1
        src/App.cpp
)
  target_link_libraries(Example1 PRIVATE Hush::Engine)
  
  # Copy resources folder to binary directory
  add_custom_command(TARGET Example1 POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/resources $<TARGET_FILE_DIR:Example1>/resources
          DEPENDS Example1
          COMMENT "Copying resources to output directory"
  )

endif()
