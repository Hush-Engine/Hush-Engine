#Vulkan
find_package(Vulkan REQUIRED)

# fmt
find_package(fmt REQUIRED)

# magic-enum
find_package(magic_enum CONFIG REQUIRED)

# SDL2
find_package(SDL2 CONFIG REQUIRED)

# SPDLOG
find_package(spdlog CONFIG REQUIRED)

# Volk
find_package(volk CONFIG REQUIRED)

# vcpkg bootstrap
find_package(vk-bootstrap CONFIG REQUIRED)

#glm
find_package(glm CONFIG REQUIRED)

#fastgltf
find_package(fastgltf CONFIG REQUIRED)

# Catch2
find_package(Catch2 CONFIG REQUIRED)
include(Catch)

# Flecs
find_package(flecs CONFIG REQUIRED)

# SPIR-V reflect
find_package(unofficial-spirv-reflect CONFIG REQUIRED)

# RapidJSON
find_package(RapidJSON CONFIG REQUIRED)

# Boost unordered
find_package(boost_unordered REQUIRED CONFIG)

# Slang shader compiler
find_package(slang CONFIG REQUIRED)

# Tracy profiler
if (HUSH_ENABLE_PROFILING)
    find_package(Tracy CONFIG REQUIRED)
endif()
