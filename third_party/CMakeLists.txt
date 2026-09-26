add_subdirectory(coreclr)
add_subdirectory(imgui)
add_subdirectory(stb)
add_subdirectory(zadeh)

# bc7enc — BCn texture compression (vendored, public domain)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/bc7enc/bc7enc.cpp")
    add_subdirectory(bc7enc)
endif()

set(WGPU_VERSION "v27.0.4.0")
set(WEBGPU_LINK_TYPE "STATIC")
if (EMSCRIPTEN)
    set(WEBGPU_BACKEND "EMDAWNWEBGPU")
else()
    set(WEBGPU_BACKEND "WGPU")
endif()
add_subdirectory(WebGPU-distribution)
if (EMSCRIPTEN)
    # Host-side reflection/binding tools do not run emcc's --use-port logic.
    # Expose the C headers explicitly alongside the distribution's C++ wrapper.
    FetchContent_GetProperties(emdawnwebgpu SOURCE_DIR HUSH_EMDAWNWEBGPU_SOURCE_DIR)
    target_include_directories(webgpu INTERFACE
        "${HUSH_EMDAWNWEBGPU_SOURCE_DIR}/webgpu/include"
    )
endif()
# TODO: migrate to SDL3
add_subdirectory(sdl3webgpu)
# add_subdirectory(sdl2webgpu)
