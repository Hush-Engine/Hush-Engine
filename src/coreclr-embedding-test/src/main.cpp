#include <cstdint>
#include <cstddef>
#include <coreclr/coreclr_delegates.h>
#include <coreclr/hostfxr.h>
#include <unistd.h>
#include <dlfcn.h>
#include <cstdio>
#include <filesystem>
#include <iostream>

template <class T>
T load_symbol(void *shared_library, const char *name)
{
  return reinterpret_cast<T>(dlsym(shared_library, name));
}

struct DemoStruct
{
  int32_t a;
  int32_t b;
};

int main()
{
  // Declare function pointers for the coreclr functions
  hostfxr_initialize_for_dotnet_command_line_fn cmd_line_fptr;
  hostfxr_initialize_for_runtime_config_fn init_fptr;
  hostfxr_get_runtime_delegate_fn get_delegate_fptr;
  hostfxr_run_app_fn run_app_fptr;
  hostfxr_close_fn close_fptr;
  hostfxr_set_error_writer_fn error_writer_fptr;

  auto shared_library = dlopen("/usr/share/dotnet/host/fxr/8.0.0/libhostfxr.so", RTLD_LAZY);
  if (!shared_library)
  {
    fputs("Failed to load libhostfxr.so\n", stderr);
    return -1;
  }

  cmd_line_fptr = load_symbol<hostfxr_initialize_for_dotnet_command_line_fn>(shared_library, "hostfxr_initialize_for_dotnet_command_line");
  init_fptr = load_symbol<hostfxr_initialize_for_runtime_config_fn>(shared_library, "hostfxr_initialize_for_runtime_config");
  get_delegate_fptr = load_symbol<hostfxr_get_runtime_delegate_fn>(shared_library, "hostfxr_get_runtime_delegate");
  run_app_fptr = load_symbol<hostfxr_run_app_fn>(shared_library, "hostfxr_run_app");
  close_fptr = load_symbol<hostfxr_close_fn>(shared_library, "hostfxr_close");
  error_writer_fptr = load_symbol<hostfxr_set_error_writer_fn>(shared_library, "hostfxr_set_error_writer");

  error_writer_fptr([](const char *message) { fputs(message, stderr); });


  auto runtime_config = "assembly-test.runtimeconfig.json";

  // Load and initialize .NET Core
  void *hostfxr_handle;
  int rc = init_fptr(runtime_config, nullptr, &hostfxr_handle);
  if (rc != 0)
  {
    fputs("Init failed\n", stderr);
    return -1;
  }

  // Get the load_assembly_and_get_function_pointer function pointer
  load_assembly_fn load_assembly;
  rc = get_delegate_fptr(
    hostfxr_handle,
    hdt_load_assembly,
    reinterpret_cast<void**>(&load_assembly));

  if (rc != 0)
  {
    fputs("Get delegate failed\n", stderr);
    return -1;
  }

  get_function_pointer_fn get_function_pointer;
  rc = get_delegate_fptr(
    hostfxr_handle,
    hdt_get_function_pointer,
    reinterpret_cast<void**>(&get_function_pointer));

  if (rc != 0)
  {
    fputs("Get delegate failed\n", stderr);
    return -1;
  }

  // Load the assembly
  auto assembly_path = std::filesystem::current_path() / "assembly-test.dll";
  std::cout << "Loading assembly: " << assembly_path << std::endl;
  rc = load_assembly(
    assembly_path.c_str(),
    nullptr,
    nullptr
  );

  if (rc != 0)
  {
    fputs("Load assembly failed\n", stderr);
    return -1;
  }

  // Get the function pointer for the managed code method
  using test_delegate_fn = void (*)();

  test_delegate_fn test_delegate;
  rc = get_function_pointer(
    "Test.Class1, assembly-test",
    "Func",
    UNMANAGEDCALLERSONLY_METHOD,
    nullptr,
    nullptr,
    reinterpret_cast<void**>(&test_delegate));

  if (rc != 0)
  {
    fputs("Get function pointer failed\n", stderr);
    return -1;
  }

  using dotnet_version_fn = void (*)(char*, int32_t);
  dotnet_version_fn dotnet_version;
  rc = get_function_pointer(
    "Test.Class1, assembly-test",
    "GetDotNetVersion",
    UNMANAGEDCALLERSONLY_METHOD,
    nullptr,
    nullptr,
    reinterpret_cast<void**>(&dotnet_version));

  if (rc != 0)
  {
    fputs("Get function pointer failed\n", stderr);
    return -1;
  }

  char version[100];
  dotnet_version(version, 100);
  std::cout << "DotNetVersion: " << version << std::endl;

  using test_struct_fn = void (*)(DemoStruct*);
  test_struct_fn test_struct;
  rc = get_function_pointer(
    "Test.Class1, assembly-test",
    "GetStruct",
    UNMANAGEDCALLERSONLY_METHOD,
    nullptr,
    nullptr,
    reinterpret_cast<void**>(&test_struct));

  if (rc != 0)
  {
    fputs("Get function pointer failed\n", stderr);
    return -1;
  }

  DemoStruct demo_struct;
  demo_struct.a = 1;
  demo_struct.b = 2;
  test_struct(&demo_struct);
  std::cout << "DemoStruct: " << demo_struct.a << ", " << demo_struct.b << std::endl;
  // Run the managed code
  test_delegate();

  // Clean up
  close_fptr(hostfxr_handle);

  return 0;
}
