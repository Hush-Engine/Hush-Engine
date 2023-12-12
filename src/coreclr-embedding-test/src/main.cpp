#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <dlfcn.h>
#include <filesystem>
#include <iostream>
#include <unistd.h>

template <class T> T LoadSymbol(void *sharedLibrary, const char *name)
{
    return reinterpret_cast<T>(dlsym(sharedLibrary, name));
}

struct DemoStruct
{
    int32_t a;
    int32_t b;
};

int main()
{
    // Declare function pointers for the coreclr functions
    hostfxr_initialize_for_dotnet_command_line_fn cmdLineFptr = nullptr;
    hostfxr_initialize_for_runtime_config_fn initFptr = nullptr;
    hostfxr_get_runtime_delegate_fn getDelegateFptr = nullptr;
    hostfxr_run_app_fn runAppFptr = nullptr;
    hostfxr_close_fn closeFptr = nullptr;
    hostfxr_set_error_writer_fn errorWriterFptr = nullptr;

    auto *sharedLibrary = dlopen("/usr/share/dotnet/host/fxr/8.0.0/libhostfxr.so", RTLD_LAZY);
    if (sharedLibrary == nullptr)
    {
        fputs("Failed to load libhostfxr.so\n", stderr);
        return -1;
    }

    cmdLineFptr = LoadSymbol<hostfxr_initialize_for_dotnet_command_line_fn>(
        sharedLibrary, "hostfxr_initialize_for_dotnet_command_line");
    (void)cmdLineFptr; // unused (for now)
    initFptr =
        LoadSymbol<hostfxr_initialize_for_runtime_config_fn>(sharedLibrary, "hostfxr_initialize_for_runtime_config");
    getDelegateFptr = LoadSymbol<hostfxr_get_runtime_delegate_fn>(sharedLibrary, "hostfxr_get_runtime_delegate");
    runAppFptr = LoadSymbol<hostfxr_run_app_fn>(sharedLibrary, "hostfxr_run_app");
    (void)runAppFptr; // unused (for now)
    closeFptr = LoadSymbol<hostfxr_close_fn>(sharedLibrary, "hostfxr_close");
    errorWriterFptr = LoadSymbol<hostfxr_set_error_writer_fn>(sharedLibrary, "hostfxr_set_error_writer");

    errorWriterFptr([](const char *message) { fputs(message, stderr); });

    const auto *runtimeConfig = "assembly-test.runtimeconfig.json";

    // Load and initialize .NET Core
    void *hostfxrHandle = nullptr;
    int rc = initFptr(runtimeConfig, nullptr, &hostfxrHandle);
    if (rc != 0)
    {
        fputs("Init failed\n", stderr);
        return -1;
    }

    // Get the load_assembly_and_get_function_pointer function pointer
    load_assembly_fn loadAssembly = nullptr;
    rc = getDelegateFptr(hostfxrHandle, hdt_load_assembly, reinterpret_cast<void **>(&loadAssembly));

    if (rc != 0)
    {
        fputs("Get delegate failed\n", stderr);
        return -1;
    }

    get_function_pointer_fn getFunctionPointer = nullptr;
    rc = getDelegateFptr(hostfxrHandle, hdt_get_function_pointer, reinterpret_cast<void **>(&getFunctionPointer));

    if (rc != 0)
    {
        fputs("Get delegate failed\n", stderr);
        return -1;
    }

    // Load the assembly
    auto assemblyPath = std::filesystem::current_path() / "assembly-test.dll";
    std::cout << "Loading assembly: " << assemblyPath << std::endl;
    rc = loadAssembly(assemblyPath.c_str(), nullptr, nullptr);

    if (rc != 0)
    {
        fputs("Load assembly failed\n", stderr);
        return -1;
    }

    // Get the function pointer for the managed code method
    using test_delegate_fn = void (*)();

    test_delegate_fn testDelegate = nullptr;
    rc = getFunctionPointer("Test.Class1, assembly-test", "Func", UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr,
                            reinterpret_cast<void **>(&testDelegate));

    if (rc != 0)
    {
        fputs("Get function pointer failed\n", stderr);
        return -1;
    }

    using dotnet_version_fn = void (*)(char *, int32_t);
    dotnet_version_fn dotnetVersion = nullptr;
    rc = getFunctionPointer("Test.Class1, assembly-test", "GetDotNetVersion", UNMANAGEDCALLERSONLY_METHOD, nullptr,
                            nullptr, reinterpret_cast<void **>(&dotnetVersion));

    if (rc != 0)
    {
        fputs("Get function pointer failed\n", stderr);
        return -1;
    }

    char version[100];
    dotnetVersion(&version[0], 100);
    std::cout << "DotNetVersion: " << &version[0] << std::endl;

    using test_struct_fn = void (*)(DemoStruct *);
    test_struct_fn testStruct = nullptr;
    rc = getFunctionPointer("Test.Class1, assembly-test", "GetStruct", UNMANAGEDCALLERSONLY_METHOD, nullptr, nullptr,
                            reinterpret_cast<void **>(&testStruct));

    if (rc != 0)
    {
        fputs("Get function pointer failed\n", stderr);
        return -1;
    }

    DemoStruct demoStruct{};
    demoStruct.a = 1;
    demoStruct.b = 2;
    testStruct(&demoStruct);
    std::cout << "DemoStruct: " << demoStruct.a << ", " << demoStruct.b << std::endl;
    // Run the managed code
    testDelegate();

    // Clean up
    closeFptr(hostfxrHandle);

    return 0;
}
