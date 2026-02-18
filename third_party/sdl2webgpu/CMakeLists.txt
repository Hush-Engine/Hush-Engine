# SDL2
find_package(SDL2 CONFIG REQUIRED)


add_library(sdl2webgpu STATIC src/sdl2webgpu/sdl2webgpu.c)

target_link_libraries(sdl2webgpu PUBLIC SDL2::SDL2 webgpu)
target_include_directories(sdl2webgpu PUBLIC src)
