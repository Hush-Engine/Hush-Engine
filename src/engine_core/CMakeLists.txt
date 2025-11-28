include(cmake/deps.cmake)
include(../cmake/utils.cmake)

add_subdirectory(app_loader)
add_subdirectory(input)
add_subdirectory(log)
add_subdirectory(rendering)
add_subdirectory(utils)
add_subdirectory(scripting)
add_subdirectory(resources)
add_subdirectory(filesystem)
add_subdirectory(threading)
add_subdirectory(core)

add_library(HushLibs INTERFACE)

target_link_libraries(HushLibs INTERFACE
        Hush::AppLoader $<TARGET_OBJECTS:HushAppLoader>
        Hush::Input $<TARGET_OBJECTS:HushInput>
        Hush::Log $<TARGET_OBJECTS:HushLog>
        Hush::Rendering $<TARGET_OBJECTS:HushRendering>
        Hush::Utils $<TARGET_OBJECTS:HushUtils>
        Hush::Resources $<TARGET_OBJECTS:HushResources>
        Hush::Filesystem $<TARGET_OBJECTS:HushFilesystem>
        Hush::Threading $<TARGET_OBJECTS:HushThreading>
        Hush::Core $<TARGET_OBJECTS:HushCore>
)

add_library(Hush::Libs ALIAS HushLibs)
add_library(Hush::ModuleLibs ALIAS HushLibs)
