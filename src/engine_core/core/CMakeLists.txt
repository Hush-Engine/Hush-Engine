
hush_add_library(
        TARGET_NAME HushCore
        LIB_TYPE OBJECT
        SRCS src/ISystem.cpp
             src/Scene.cpp
             src/Entity.cpp
             src/Components/Transform.cpp
             src/Query.cpp
             src/serialization/Serialization.cpp
             src/serialization/Deserialization.cpp
             src/serialization/Formats/JsonSerializer.cpp
             src/traits/EntityTraits.cpp
             src/reflection/Type.cpp
             src/reflection/Variant.cpp
        PUBLIC_HEADER_DIRS src
        PRIVATE_HEADER_DIRS private
        ENABLE_REFLECTION ON
)
add_library(Hush::Core ALIAS HushCore)

target_link_libraries(HushCore PUBLIC flecs::flecs_static Hush::Utils Hush::Log zadeh rapidjson absl::base absl::flat_hash_map)

add_test_target(
        TARGET_NAME HushCoreTest
        ENGINE_TARGET HushCore
        SRCS tests/Entity.test.cpp tests/Query.test.cpp tests/Reflection.test.cpp tests/Serialization.test.cpp
        HEADER_DIRS tests
        ENABLE_REFLECTION ON
)
