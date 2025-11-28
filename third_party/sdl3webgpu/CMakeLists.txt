# SDL3
find_package(SDL3 CONFIG REQUIRED)


add_library(sdl3webgpu STATIC src/sdl3webgpu/sdl3webgpu.c)

target_link_libraries(sdl3webgpu SDL3::SDL3)
target_include_directories(sdl3webgpu PUBLIC src)
