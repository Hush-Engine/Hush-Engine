# Memory support subproject

hush_add_library(
        TARGET_NAME HushMemory
        LIB_TYPE OBJECT
        SRCS src/Hush/Memory/ThreadLocalMemoryResourcePool.cpp
             src/Hush/Memory/CoroutineFrameResource.cpp
        PUBLIC_HEADER_DIRS src
)
add_library(Hush::Memory ALIAS HushMemory)

target_link_libraries(HushMemory PUBLIC Hush::Log Hush::Utils)

add_test_target(
        TARGET_NAME HushMemoryTest
        ENGINE_TARGET HushMemory
        SRCS tests/ThreadLocalMemoryResourcePool.test.cpp
             tests/CoroutineFrameResource.test.cpp
             tests/MakeNullTerminated.test.cpp
             tests/AllocatorBenchmark.test.cpp
        HEADER_DIRS tests
)

# Link mimalloc into the benchmark so AllocatorBenchmark can compare the coroutine pool against
# mimalloc directly (the real baseline: un-pooled coroutine frames hit mimalloc via the global
# operator new override). The test executable itself is not minject'd, so ::operator new is CRT
# malloc — mi_malloc/mi_free give the apples-to-apples number. Gated on mimalloc being available
# (it always is via vcpkg on non-emscripten), independent of the engine's HUSH_ENABLE_MIMALLOC.
find_package(mimalloc CONFIG QUIET)
if (TARGET mimalloc OR TARGET mimalloc-static)
    target_link_libraries(HushMemoryTest PRIVATE
            $<IF:$<TARGET_EXISTS:mimalloc-static>,mimalloc-static,mimalloc>)
    target_compile_definitions(HushMemoryTest PRIVATE HUSH_USE_MIMALLOC=1)
endif()
