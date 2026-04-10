message("*****Compiling ImGui*****")

message("Getting dependencies...")
find_package(SDL2 CONFIG REQUIRED)

set(BASE_INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/include/imgui")
set(GUIZMO_BASE_INCLUDE_PATH "${CMAKE_CURRENT_LIST_DIR}/include/imguizmo")

file(GLOB IMGUI_BASE_SRC "${BASE_INCLUDE_PATH}/*.h" "${BASE_INCLUDE_PATH}/*.cpp")
file(GLOB IMGUIZMO_BASE_SRC "${GUIZMO_BASE_INCLUDE_PATH}/*.h" "${GUIZMO_BASE_INCLUDE_PATH}/*.cpp")

set(IMGUI_SOURCES
 ${BASE_INCLUDE_PATH}/backends/imgui_impl_sdl2.cpp
 ${BASE_INCLUDE_PATH}/backends/imgui_impl_sdl2.h
 # ${BASE_INCLUDE_PATH}/backends/imgui_impl_vulkan.cpp
 # ${BASE_INCLUDE_PATH}/backends/imgui_impl_vulkan.h
 ${BASE_INCLUDE_PATH}/backends/imgui_impl_wgpu.cpp
 ${BASE_INCLUDE_PATH}/backends/imgui_impl_wgpu.h
 ${IMGUI_BASE_SRC}
)

set(IMGUIZMO_SOURCES
 "${GUIZMO_BASE_INCLUDE_PATH}/ImGuizmo.h"
 "${GUIZMO_BASE_INCLUDE_PATH}/ImGuizmo.cpp"
)

message("ImGui base sources")

add_library(imgui STATIC ${IMGUI_SOURCES})
add_library(imguizmo STATIC ${IMGUIZMO_SOURCES})

message("Linking dependencies...")
target_link_libraries(imgui PUBLIC SDL2::SDL2 webgpu)

target_compile_definitions(imgui PUBLIC IMGUI_IMPL_WEBGPU_BACKEND_WGPU)

target_include_directories(imgui PUBLIC include include/imgui)
target_include_directories(imguizmo PUBLIC include)

message("*****ImGui End*****")
