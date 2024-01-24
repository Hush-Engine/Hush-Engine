#include "core/scripting/DotnetHost.hpp"

constexpr std::string_view DOTNET_CMD("hostfxr_initialize_for_dotnet_command_line");
constexpr std::string_view DOTNET_RUNTIME_INIT_CONFIG("hostfxr_initialize_for_runtime_config");
constexpr std::string_view DOTNET_RUNTIME_DELEGATE("hostfxr_get_runtime_delegate");
constexpr std::string_view DOTNET_RUN_FUNCTION("hostfxr_run_app");
constexpr std::string_view DOTNET_CLOSE_FUNCTION("hostfxr_close");
constexpr std::string_view DOTNET_ERROR_WRITER("hostfxr_set_error_writer");

constexpr std::string_view RUNTIME_CONFIG_JSON("assembly-test.runtimeconfig.json");

#if WIN32
#define HOST_FXR_PATH "host/fxr/8.0.0/hostfxr.dll"
#elif __linux__
#define HOST_FXR_PATH "host/fxr/8.0.0/libhostfxr.so"
#elif __APPLE__
#define HOST_FXR_PATH "host/fxr/8.0.0/libhostfxr.dylib"
#endif

DotnetHost::DotnetHost(const char *dotnetPath)
{
    std::filesystem::path targetPath = dotnetPath;
    targetPath /= HOST_FXR_PATH;
#if WIN32
    std::string strLibPath = targetPath.string();
    const char *libPath = strLibPath.c_str();
#else
    const char *libPath = targetPath.c_str();
#endif
    void *sharedLibrary = LibManager::LibraryOpen(libPath);
    if (sharedLibrary == nullptr)
    {
        LOG_ERROR_LN("Failed to load %s", libPath);
        return;
    }
    // TODO: See which of these can stop being cached and just pass them as params to the initdotnetcore
    this->m_cmdLineFuncPtr =
        LoadSymbol<hostfxr_initialize_for_dotnet_command_line_fn>(sharedLibrary, DOTNET_CMD.data());
    this->m_initFuncPtr =
        LoadSymbol<hostfxr_initialize_for_runtime_config_fn>(sharedLibrary, DOTNET_RUNTIME_INIT_CONFIG.data());
    this->m_getDelegateFuncPtr =
        LoadSymbol<hostfxr_get_runtime_delegate_fn>(sharedLibrary, DOTNET_RUNTIME_DELEGATE.data());
    this->m_runAppFuncPtr = LoadSymbol<hostfxr_run_app_fn>(sharedLibrary, DOTNET_RUN_FUNCTION.data());
    this->m_closeFuncPtr = LoadSymbol<hostfxr_close_fn>(sharedLibrary, DOTNET_CLOSE_FUNCTION.data());
    this->m_errorWriterFuncPtr = LoadSymbol<hostfxr_set_error_writer_fn>(sharedLibrary, DOTNET_ERROR_WRITER.data());
    // Add logging for any errors in C#
#if WIN32
    this->m_errorWriterFuncPtr([](const char_t *message) {
        std::wstring wStrMessage = message;
        std::string strMessage = StringUtils::FromWString(wStrMessage);
        const char *cMessage = strMessage.c_str();
        LOG_ERROR_LN("Received an error from C# runtime %s", cMessage);
    });
#else
    this->m_errorWriterFuncPtr(
        [](const char *message) { LOG_ERROR_LN("Received an error from C# runtime %s", message); });
#endif
    this->InitDotnetCore();
}

DotnetHost::DotnetHost(DotnetHost &&other) noexcept
    : m_cmdLineFuncPtr(other.m_cmdLineFuncPtr), m_initFuncPtr(other.m_initFuncPtr),
      m_getDelegateFuncPtr(other.m_getDelegateFuncPtr), m_runAppFuncPtr(other.m_runAppFuncPtr),
      m_closeFuncPtr(other.m_closeFuncPtr), m_errorWriterFuncPtr(other.m_errorWriterFuncPtr),
      m_functionGetterFuncPtr(other.m_functionGetterFuncPtr), m_hostFxrHandle(other.m_hostFxrHandle)
{
    other.m_cmdLineFuncPtr = nullptr;
    other.m_initFuncPtr = nullptr;
    other.m_getDelegateFuncPtr = nullptr;
    other.m_runAppFuncPtr = nullptr;
    other.m_closeFuncPtr = nullptr;
    other.m_errorWriterFuncPtr = nullptr;
    other.m_functionGetterFuncPtr = nullptr;
    other.m_hostFxrHandle = nullptr;
}

DotnetHost::~DotnetHost()
{
    if (this->m_closeFuncPtr != nullptr)
    {
        this->m_closeFuncPtr(this->m_hostFxrHandle);
    }
}

DotnetHost &DotnetHost::operator=(DotnetHost &&other) noexcept
{
    this->m_cmdLineFuncPtr = other.m_cmdLineFuncPtr;
    this->m_initFuncPtr = other.m_initFuncPtr;
    this->m_getDelegateFuncPtr = other.m_getDelegateFuncPtr;
    this->m_runAppFuncPtr = other.m_runAppFuncPtr;
    this->m_closeFuncPtr = other.m_closeFuncPtr;
    this->m_errorWriterFuncPtr = other.m_errorWriterFuncPtr;
    this->m_functionGetterFuncPtr = other.m_functionGetterFuncPtr;
    this->m_hostFxrHandle = other.m_hostFxrHandle;

    other.m_cmdLineFuncPtr = nullptr;
    other.m_initFuncPtr = nullptr;
    other.m_getDelegateFuncPtr = nullptr;
    other.m_runAppFuncPtr = nullptr;
    other.m_closeFuncPtr = nullptr;
    other.m_errorWriterFuncPtr = nullptr;
    other.m_functionGetterFuncPtr = nullptr;
    other.m_hostFxrHandle = nullptr;
    return *this;
}

get_function_pointer_fn DotnetHost::GetFunctionGetterFuncPtr()
{
    return this->m_functionGetterFuncPtr;
}

void DotnetHost::InitDotnetCore()
{
    std::filesystem::path runtimeConfigPath = LibManager::GetCurrentExecutablePath();
    runtimeConfigPath /= RUNTIME_CONFIG_JSON.data();
#if WIN32
    std::wstring configStr = runtimeConfigPath.wstring();
    const char_t *runtimeConfig = configStr.data();
#else
    const char *runtimeConfig = runtimeConfigPath.c_str();
#endif

    // Load and initialize .NET Core
    int rc = this->m_initFuncPtr(runtimeConfig, nullptr, &this->m_hostFxrHandle);
    if (rc != 0)
    {
        LOG_ERROR_LN("Init failed");
        return;
    }
    LOG_INFO_LN("Init .NET core succeeded!");
    load_assembly_fn assemblyLoader = this->GetLoadAssembly(this->m_hostFxrHandle);

    if (assemblyLoader == nullptr)
    {
        LOG_ERROR_LN("Could not get load assembly ptr");
        return;
    }

    this->m_functionGetterFuncPtr = this->GetFunctionPtr(this->m_hostFxrHandle);

    if (this->m_functionGetterFuncPtr == nullptr)
    {
        LOG_ERROR_LN("Get function ptr failed");
        return;
    }
    // Actually load the assembly
    bool isAssemblyLoaded = LoadAssemblyFromPath(assemblyLoader);

    if (!isAssemblyLoaded)
    {
        LOG_ERROR_LN("Failed to load the assembly");
        return;
    }
}

load_assembly_fn DotnetHost::GetLoadAssembly(void *hostFxrHandle)
{
    // Get the load_assembly_and_get_function_pointer function pointer
    load_assembly_fn loadAssembly = nullptr;
    m_getDelegateFuncPtr(hostFxrHandle, hdt_load_assembly, reinterpret_cast<void **>(&loadAssembly));
    return loadAssembly;
}

get_function_pointer_fn DotnetHost::GetFunctionPtr(void *hostFxrHandle)
{
    get_function_pointer_fn getFunctionPointer = nullptr;
    m_getDelegateFuncPtr(hostFxrHandle, hdt_get_function_pointer, reinterpret_cast<void **>(&getFunctionPointer));
    return getFunctionPointer;
}

bool DotnetHost::LoadAssemblyFromPath(load_assembly_fn assemblyLoader)
{
    std::filesystem::path assemblyPath = LibManager::GetCurrentExecutablePath() / "assembly-test.dll";
    int rc = assemblyLoader(assemblyPath.c_str(), nullptr, nullptr);
    return rc == 0;
}

template <class T> T DotnetHost::LoadSymbol(void *sharedLibrary, const char *name)
{
    void *libraryPtr = LibManager::DynamicLoadSymbol(sharedLibrary, name);
    return reinterpret_cast<T>(libraryPtr);
}
