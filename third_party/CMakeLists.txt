add_subdirectory(coreclr)
add_subdirectory(imgui)
add_subdirectory(stb)
add_subdirectory(zadeh)

set(WEBGPU_LINK_TYPE "STATIC")
add_subdirectory(WebGPU-distribution)
# TODO: migrate to SDL3
#add_subdirectory(sdl3webgpu)
add_subdirectory(sdl2webgpu)
